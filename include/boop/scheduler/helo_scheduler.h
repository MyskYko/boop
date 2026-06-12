#pragma once

#include <algorithm>
#include <cassert>
#include <functional>
#include <iostream>
#include <map>
#include <queue>
#include <random>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "boop/config.h"

#ifdef BOOP_USE_THREADS
#include <condition_variable>
#include <mutex>
#include <thread>
#endif

#include "boop/util/util.h"
#include "boop/interface/abc_interface.h"

BOOP_HEADER_START

namespace boop {

  template <typename Ntk, typename Opt, typename Prt>
  class HeloScheduler {
  public:
    struct Parameter {
      // objective function (lower is better)
      std::function<Cost(Ntk *)> fnObjective = [](Ntk *pNtk) {
        return pNtk->GetNumFanins() - pNtk->GetNumInts();
      };

      int nVerbose = 0;
      int nSeed = 0;
      int nFlow = 0;
      int nJobs = 1;
      int nThreads = 1;
      bool fDeterministic = true;
      int nTimeout = 0;
      
      bool fPartitioning = false;
      int nParallelPartitions = 1;
      bool fOptOnInsert = false;

      typename Opt::Parameter parOpt;
      typename Prt::Parameter parPrt;
    };

    // lifecycle
    HeloScheduler(Ntk *pNtk, Parameter const &par);
    ~HeloScheduler();

    // run
    void Run();
    
  private:
    static constexpr const char *pCompress2rs = "balance -l; resub -K 6 -l; rewrite -l; resub -K 6 -N 2 -l; refactor -l; resub -K 8 -l; balance -l; resub -K 8 -N 2 -l; rewrite -l; resub -K 10 -l; rewrite -z -l; resub -K 10 -N 2 -l; balance -l; resub -K 12 -l; refactor -z -l; resub -K 12 -N 2 -l; rewrite -z -l; balance -l";
    
    struct Job {
      int nId;
      Ntk *pNtk;
      int nSeed;
      Cost costInitial;
      std::string strPrefix;
      Duration duration;
      Summary<int> summaryStats;
      Summary<Duration> summaryTimes;
      
      Job(int nId, Ntk *pNtk, int nSeed, Cost costInitial)
        : nId(nId),
          pNtk(pNtk),
          nSeed(nSeed),
          costInitial(costInitial) {
        std::stringstream ss;
        PrintNext(ss, "job", nId, ":");
        strPrefix = ss.str() + " ";
      }
    };
    
    struct CompareJobPointers {
      bool operator()(Job const *lhs, Job const *rhs) const {
        return lhs->nId > rhs->nId;
      }
    };

    Ntk *pNtk_;

    const Parameter par_;

    bool fMultithreaded_;
    bool fDeterministic_;

    Prt prt_;
    Opt *pOpt_; // used only in case of single thread execution

    int nCreatedJobs_;
    int nFinishedJobs_;
    TimePoint timeStart_;
    Cost costStart_;

    std::queue<Job *> qPendingJobs_;

    std::vector<std::string> vStatsKeys_;
    std::map<std::string, int> mStatsSummary_;
    std::vector<std::string> vTimesKeys_;
    std::map<std::string, Duration> mTimesSummary_;

#ifdef BOOP_USE_THREADS
    bool fTerminate_;
    std::vector<std::thread> vThreads_;
    std::priority_queue<Job *, std::vector<Job *>, CompareJobPointers> qFinishedJobs_;
    std::mutex mutexAbc_;
    std::mutex mutexPendingJobs_;
    std::mutex mutexFinishedJobs_;
    std::mutex mutexPrint_;
    std::condition_variable condPendingJobs_;
    std::condition_variable condFinishedJobs_;
#endif

    // print
    template <typename... Args>
    void Print(int nVerboseLevel, const std::string &strPrefix, Args &&...args);
    std::string MakeNtkInfoString(const Ntk *pNtk, Cost cost) const;
    std::string MakeStepInfoString(const Ntk *pNtk, Cost cost, Cost costInitial, Duration duration) const;
    void PrintSummary();

    // time
    Seconds GetRemainingTime() const;
    Duration GetElapsedTime() const;

    // abc
    void CallAbc(Ntk *pNtk, std::string command, Duration &duration);

    // execute jobs
    void ExecuteTranstochFlow(Opt &opt, Job *pJob, Duration &durationAbc);
    void ExecuteDeepFlow(Opt &opt, Job *pJob, Duration &durationAbc);
    void ExecuteAbcLoopFlow(Opt &opt, Job *pJob, Duration &durationAbc);
    void ExecuteJob(Opt &opt, Job *pJob);

    // manage jobs
    Job *CreateJob(Ntk *pNtk, int nSeed, Cost cost);
    void OnJobEnd(const std::function<void(Job *pJob)> &fn);

    // thread
#ifdef BOOP_USE_THREADS
    void Thread(const typename Opt::Parameter &parOpt);
#endif

    // summary
    template <typename T>
    void AddToSummary(std::vector<std::string> &vKeys, std::map<std::string, T> &mSummary, const Summary<T> &summary) const;

    // run helpers
    void RunWithPartitioning();
    void RunMultipleJobs();
    void RunSingleJob();
  };

  // lifecycle
  
  template <typename Ntk, typename Opt, typename Prt>
  HeloScheduler<Ntk, Opt, Prt>::HeloScheduler(Ntk *pNtk, Parameter const &par)
    : pNtk_(pNtk),
      par_(par),
      fMultithreaded_(par_.nThreads > 1),
      fDeterministic_(par_.fDeterministic),
      prt_(par_.parPrt),
      pOpt_(nullptr),
      nCreatedJobs_(0),
      nFinishedJobs_(0) {
#ifdef BOOP_USE_THREADS
    fTerminate_ = false;
    if(fMultithreaded_) {
      vThreads_.reserve(par_.nThreads);
      for(int i = 0; i < par_.nThreads; i++) {
        vThreads_.emplace_back([this, parOpt = par_.parOpt]() {
          Thread(parOpt);
        });
      }
      return;
    }
#endif
    assert(!fMultithreaded_);
    pOpt_ = new Opt(par_.parOpt, par_.fnObjective);
    StartAbc();
  }

  template <typename Ntk, typename Opt, typename Prt>
  HeloScheduler<Ntk, Opt, Prt>::~HeloScheduler() {
#ifdef BOOP_USE_THREADS
    if(fMultithreaded_) {
      {
        std::unique_lock<std::mutex> l(mutexPendingJobs_);
        fTerminate_ = true;
        condPendingJobs_.notify_all();
      }
      for(std::thread &t: vThreads_) {
        t.join();
      }
      return;
    }
#endif
    delete pOpt_;
    StopAbc();
  }

  // run
  
  template <typename Ntk, typename Opt, typename Prt>
  void HeloScheduler<Ntk, Opt, Prt>::Run() {
    timeStart_ = GetCurrentTime();
    costStart_ = par_.fnObjective(pNtk_);
    if(par_.fPartitioning) {
      RunWithPartitioning();
    } else if(par_.nJobs > 1) {
      RunMultipleJobs();
    } else {
      RunSingleJob();
    }
    PrintSummary();
  }
  
  template <typename Ntk, typename Opt, typename Prt>
  template <typename... Args>
  void HeloScheduler<Ntk, Opt, Prt>::Print(int nVerboseLevel, const std::string &strPrefix, Args &&...args) {
    if(par_.nVerbose <= nVerboseLevel) {
      return;
    }
#ifdef BOOP_USE_THREADS
    if(fMultithreaded_) {
      {
        std::unique_lock<std::mutex> l(mutexPrint_);
        std::cout << strPrefix;
        PrintNext(std::cout, std::forward<Args>(args)...);
        std::cout << std::endl;
      }
      return;
    }
#endif
    std::cout << strPrefix;
    PrintNext(std::cout, std::forward<Args>(args)...);
    std::cout << std::endl;
  }

  template <typename Ntk, typename Opt, typename Prt>
  std::string HeloScheduler<Ntk, Opt, Prt>::MakeNtkInfoString(const Ntk *pNtk, Cost cost) const {
    std::stringstream ss;
    PrintNext(ss, "i/o", "=", pNtk->GetNumPis(), "/", pNtk->GetNumPos(), ",",
              "node", "=", pNtk->GetNumInts(), ",",
              "level", "=", pNtk->GetNumLevels(), ",",
              "cost", "=", cost);
    return ss.str();
  }

  template <typename Ntk, typename Opt, typename Prt>
  std::string HeloScheduler<Ntk, Opt, Prt>::MakeStepInfoString(const Ntk *pNtk, Cost cost, Cost costInitial, Duration duration) const {
    std::stringstream ss;
    PrintNext(ss, MakeNtkInfoString(pNtk, cost),
              "(", 100 * (cost - costInitial) / costInitial, "%", ")", ",",
              "duration", "=", duration, "s", ",",
              "elapsed", "=", GetElapsedTime(), "s");
    return ss.str();
  }

  template <typename Ntk, typename Opt, typename Prt>
  void HeloScheduler<Ntk, Opt, Prt>::PrintSummary() {
    Cost cost = par_.fnObjective(pNtk_);
    Duration duration = GetElapsedTime();
    Print(0, "\n", "stats summary", ":");
    for(std::string key: vStatsKeys_) {
      Print(0, "\t", SW{30, true}, key, ":", SW{10}, mStatsSummary_.at(key));
    }
    Print(0, "", "runtime summary", ":");
    for(std::string key: vTimesKeys_) {
      Print(0, "\t", SW{30, true}, key, ":", mTimesSummary_.at(key), "s", "(", 100 * mTimesSummary_.at(key) / duration, "%", ")");
    }
    Print(0, "", "end", ":", "cost", "=", cost, "(", 100 * (cost - costStart_) / costStart_, "%", ")", ",", "time", "=", duration, "s");
  }
    
  // time

  template <typename Ntk, typename Opt, typename Prt>
  Seconds HeloScheduler<Ntk, Opt, Prt>::GetRemainingTime() const {
    if(par_.nTimeout == 0) {
      return 0;
    }
    TimePoint timeCurrent = GetCurrentTime();
    Seconds nRemainingTime = par_.nTimeout - GetDurationInSeconds(timeStart_, timeCurrent);
    if(nRemainingTime == 0) { // avoid glitch
      return -1;
    }
    return nRemainingTime;
  }

  template <typename Ntk, typename Opt, typename Prt>
  Duration HeloScheduler<Ntk, Opt, Prt>::GetElapsedTime() const {
    TimePoint timeCurrent = GetCurrentTime();
    return GetDuration(timeStart_, timeCurrent);
  }

  // abc

  template <typename Ntk, typename Opt, typename Prt>
  void HeloScheduler<Ntk, Opt, Prt>::CallAbc(Ntk *pNtk, std::string command, Duration &duration) {
  #ifdef BOOP_USE_THREADS
    if(fMultithreaded_) {
      {
        std::unique_lock<std::mutex> l(mutexAbc_);
        TimePoint timeStartAbc = GetCurrentTime();
        Abc9Execute(pNtk, command);
        TimePoint timeEndAbc = GetCurrentTime();
        duration += GetDuration(timeStartAbc, timeEndAbc);
      }
      return;
    }
  #endif
    TimePoint timeStartAbc = GetCurrentTime();
    Abc9Execute(pNtk, command);
    TimePoint timeEndAbc = GetCurrentTime();
    duration += GetDuration(timeStartAbc, timeEndAbc);
  }

  // execute jobs

  template <typename Ntk, typename Opt, typename Prt>
  void HeloScheduler<Ntk, Opt, Prt>::ExecuteTranstochFlow(Opt &opt, Job *pJob, Duration &durationAbc) {
    std::mt19937 rng(pJob->nSeed);
    Cost cost = pJob->costInitial;
    Cost costBest = cost;
    int nSlot = pJob->pNtk->Save();
    for(int i = 0; i < 10; i++) {
      if(GetRemainingTime() < 0) {
        break;
      }
      if(i != 0) {
        CallAbc(pJob->pNtk, "&if -K 6; &mfs; &st", durationAbc);
        cost = par_.fnObjective(pJob->pNtk);
        Print(1, pJob->strPrefix, "hop", i, ":", "cost", "=", cost);
      }
      for(int j = 0; true; j++) {
        if(GetRemainingTime() < 0) {
          break;
        }
        opt.Run(rng(), GetRemainingTime());
        CallAbc(pJob->pNtk, "&dc2", durationAbc);
        Cost costNew = par_.fnObjective(pJob->pNtk);
        Print(1, pJob->strPrefix, "ite", j, ":", "cost", "=", costNew);
        if(costNew < cost) {
          cost = costNew;
        } else  {
          break;
        }
      }
      if(cost < costBest) {
        costBest = cost;
        pJob->pNtk->Save(nSlot);
        i = 0;
      }
    }
    pJob->pNtk->Load(nSlot);
    pJob->pNtk->PopBack();
  }

  template <typename Ntk, typename Opt, typename Prt>
  void HeloScheduler<Ntk, Opt, Prt>::ExecuteDeepFlow(Opt &opt, Job *pJob, Duration &durationAbc) {
    SimpleRNG rng;
    for(int i = 0; i < 11; i++) {
      rng(); // align with deepsyn
    }
    std::mt19937 rng2(pJob->nSeed);
    int n = 0;
    Cost cost = pJob->costInitial;
    int nSlot = pJob->pNtk->Save();
    for(int i = 0; i < 1000000; i++) {
      if(GetRemainingTime() < 0) {
        break;
      }
      bool fUseTwo = false;
      unsigned nRand = rng();
      bool fDch = nRand & 1;
      int nComp = (nRand >> 1) & 1; // align with deepsyn
      bool fFx = (nRand >> 2) & 1;
      int nLutSize = fUseTwo ? 2 + (i % 5) : 3 + (i % 4);
      std::string strComp;
      if(nComp == 3) {
        strComp = std::string("; &put; ") + pCompress2rs + "; " + pCompress2rs + "; " + pCompress2rs + "; &get";
      } else if(nComp == 2) {
        strComp = std::string("; &put; ") + pCompress2rs + "; " + pCompress2rs + "; &get";
      } else if(nComp == 1) {
        strComp = std::string("; &put; ") + pCompress2rs + "; &get";
      } else {
        strComp = "; &dc2";
      }
      std::string strCommand = "&dch";
      if(fDch) {
        strCommand += " -f";
      }
      strCommand += "; &if -a -K " + std::to_string(nLutSize) + "; &mfs -e -W 20 -L 20";
      if(fFx) {
        strCommand += "; &fx; &st";
      }
      strCommand += strComp;
      CallAbc(pJob->pNtk, strCommand, durationAbc);
      Print(1, pJob->strPrefix, "ite", i, ":", "cost", "=", par_.fnObjective(pJob->pNtk));
      for(int j = 0; j < n; j++) {
        if(GetRemainingTime() < 0) {
          break;
        }
        opt.Run(rng2(), GetRemainingTime());
        if(rng2() & 1) {
          CallAbc(pJob->pNtk, "&dc2", durationAbc);
        } else {
          CallAbc(pJob->pNtk, std::string("&put; ") + pCompress2rs + "; &get", durationAbc);
        }
        Print(1, pJob->strPrefix, "rrr", j, ":", "cost", "=", par_.fnObjective(pJob->pNtk));
      }
      Cost costNew = par_.fnObjective(pJob->pNtk);
      if(costNew < cost) {
        cost = costNew;
        pJob->pNtk->Save(nSlot);
      } else {
        n++;
      }
    }
    pJob->pNtk->Load(nSlot);
    pJob->pNtk->PopBack();
  }

  template <typename Ntk, typename Opt, typename Prt>
  void HeloScheduler<Ntk, Opt, Prt>::ExecuteAbcLoopFlow(Opt &opt, Job *pJob, Duration &durationAbc) {
    for(int i = 0; i < 100; i++) {
      if(GetRemainingTime() < 0) {
        break;
      }
      opt.Run(pJob->nSeed, GetRemainingTime());
      CallAbc(pJob->pNtk, std::string("&put; ") + pCompress2rs + "; dc2; &get", durationAbc);
    }
  }
  
  template <typename Ntk, typename Opt, typename Prt>
  void HeloScheduler<Ntk, Opt, Prt>::ExecuteJob(Opt &opt, Job *pJob) {
    TimePoint timeStartLocal = GetCurrentTime();
    opt.AssignNetwork(pJob->pNtk, !par_.fPartitioning); // reuse backend if restarting
    opt.SetPrintLine([&](const std::string &str) {
      Print(-1, pJob->strPrefix, str);
    });
    Duration durationAbc = 0;
    switch(par_.nFlow) {
    case 0:
      opt.Run(pJob->nSeed, GetRemainingTime());
      break;
    case 1:
      ExecuteTranstochFlow(opt, pJob, durationAbc);
      break;
    case 2:
      ExecuteDeepFlow(opt, pJob, durationAbc);
      break;
    case 4:
      ExecuteAbcLoopFlow(opt, pJob, durationAbc);
      break;
    default:
      assert(0);
    }
    TimePoint timeEndLocal = GetCurrentTime();
    pJob->duration = GetDuration(timeStartLocal, timeEndLocal);
    pJob->summaryStats = opt.GetStatsSummary();
    pJob->summaryTimes = opt.GetTimesSummary();
    pJob->summaryTimes.emplace_back("abc", durationAbc);
    opt.ResetSummary();
  }

  // manage jobs

  template <typename Ntk, typename Opt, typename Prt>
  typename HeloScheduler<Ntk, Opt, Prt>::Job *
  HeloScheduler<Ntk, Opt, Prt>::CreateJob(Ntk *pNtk, int nSeed, Cost cost) {
    Job *pJob = new Job(nCreatedJobs_++, pNtk, nSeed, cost);
#ifdef BOOP_USE_THREADS
    if(fMultithreaded_) {
      {
        std::unique_lock<std::mutex> l(mutexPendingJobs_);
        qPendingJobs_.push(pJob);
        condPendingJobs_.notify_one();
      }
      return pJob;
    }
#endif
    qPendingJobs_.push(pJob);
    return pJob;
  }


  template <typename Ntk, typename Opt, typename Prt>
  void HeloScheduler<Ntk, Opt, Prt>::OnJobEnd(const std::function<void(Job *pJob)> &fn) {
#ifdef BOOP_USE_THREADS
    if(fMultithreaded_) {
      Job *pJob = nullptr;
      {
        std::unique_lock<std::mutex> l(mutexFinishedJobs_);
        while(qFinishedJobs_.empty() || (fDeterministic_ && qFinishedJobs_.top()->nId != nFinishedJobs_)) {
          condFinishedJobs_.wait(l);
        }
        pJob = qFinishedJobs_.top();
        qFinishedJobs_.pop();
      }
      assert(pJob != nullptr);
      fn(pJob);
      AddToSummary(vStatsKeys_, mStatsSummary_, pJob->summaryStats);
      AddToSummary(vTimesKeys_, mTimesSummary_, pJob->summaryTimes);
      delete pJob;
      nFinishedJobs_++;
      return;
    }
#endif
    // single thread
    assert(!qPendingJobs_.empty());
    Job *pJob = qPendingJobs_.front();
    qPendingJobs_.pop();
    ExecuteJob(*pOpt_, pJob);
    fn(pJob);
    AddToSummary(vStatsKeys_, mStatsSummary_, pJob->summaryStats);
    AddToSummary(vTimesKeys_, mTimesSummary_, pJob->summaryTimes);
    delete pJob;
    nFinishedJobs_++;
  }

  // thread

#ifdef BOOP_USE_THREADS
  template <typename Ntk, typename Opt, typename Prt>
  void HeloScheduler<Ntk, Opt, Prt>::Thread(const typename Opt::Parameter &parOpt) {
    StartAbc();
    Opt opt(parOpt, par_.fnObjective);
    while(true) {
      Job *pJob = NULL;
      {
        std::unique_lock<std::mutex> l(mutexPendingJobs_);
        while(!fTerminate_ && qPendingJobs_.empty()) {
          condPendingJobs_.wait(l);
        }
        if(fTerminate_) {
          assert(qPendingJobs_.empty());
          StopAbc();
          return;
        }
        pJob = qPendingJobs_.front();
        qPendingJobs_.pop();
      }
      assert(pJob != NULL);
      ExecuteJob(opt, pJob);
      {
        std::unique_lock<std::mutex> l(mutexFinishedJobs_);
        qFinishedJobs_.push(pJob);
        condFinishedJobs_.notify_one();
      }
    }
  }
  #endif
  
  // summary

  template <typename Ntk, typename Opt, typename Prt>
  template <typename T>
  void HeloScheduler<Ntk, Opt, Prt>::AddToSummary(std::vector<std::string> &vKeys, std::map<std::string, T> &mSummary, const Summary<T> &summary) const {
    std::vector<std::string>::iterator it = vKeys.begin();
    for(const auto &entry: summary) {
      if(mSummary.count(entry.first)) {
        mSummary[entry.first] += entry.second;
        it = std::find(it, vKeys.end(), entry.first);
        assert(it != vKeys.end());
        it++;
      } else {
        mSummary[entry.first] = entry.second;
        it = vKeys.insert(it, entry.first);
        it++;
      }
    }
  }

  // run helpers

  template <typename Ntk, typename Opt, typename Prt>
  void HeloScheduler<Ntk, Opt, Prt>::RunWithPartitioning() {
    std::mt19937 rng(par_.nSeed);
    fDeterministic_ = false; // deterministic anyway (wait for all jobs each round)
    pNtk_->Sweep();
    prt_.AssignNetwork(pNtk_);
    prt_.SetPrintLine([&](const std::string &str) {
      Print(-1, "", str);
    });
    while(nCreatedJobs_ < par_.nJobs) {
      assert(par_.nParallelPartitions > 0);
      if(nCreatedJobs_ < nFinishedJobs_ + par_.nParallelPartitions) {
        Ntk *pSubNtk = prt_.Extract(rng());
        if(pSubNtk) {
          Job *pJob = CreateJob(pSubNtk, rng(), par_.fnObjective(pSubNtk));
          Print(1, pJob->strPrefix, "created", ":", MakeNtkInfoString(pJob->pNtk, pJob->costInitial));
          continue;
        }
      }
      if(nCreatedJobs_ == nFinishedJobs_) {
        PrintWarning("failed to partition");
        break;
      }
      while(nFinishedJobs_ < nCreatedJobs_) {
        OnJobEnd([&](Job *pJob) {
          Cost cost = par_.fnObjective(pJob->pNtk);
          Print(1, pJob->strPrefix, "finished", ":", MakeNtkInfoString(pJob->pNtk, cost));
          Print(0, "", "job", pJob->nId, "(", nFinishedJobs_ + 1, "/", par_.nJobs, ")", ":", MakeStepInfoString(pJob->pNtk, cost, pJob->costInitial, pJob->duration));
          prt_.Insert(pJob->pNtk);
        });
      }
      if(par_.fOptOnInsert) {
        Cost costInitial = par_.fnObjective(pNtk_);
        Duration duration = 0;
        CallAbc(pNtk_, std::string("&put; ") + pCompress2rs + "; dc2; &get", duration);
        prt_.AssignNetwork(pNtk_);
        Cost cost = par_.fnObjective(pNtk_);
        Print(0, "", "c2rs; dc2", ":", std::string(8 + 3 * PrintFormat::int_width, ' '), MakeStepInfoString(pNtk_, cost, costInitial, duration));
      } 
    }
    while(nFinishedJobs_ < nCreatedJobs_) {
      OnJobEnd([&](Job *pJob) {
        Cost cost = par_.fnObjective(pJob->pNtk);
        Print(1, pJob->strPrefix, "finished", ":", MakeNtkInfoString(pJob->pNtk, cost));
        Print(0, "", "job", pJob->nId, "(", nFinishedJobs_ + 1, "/", par_.nJobs, ")", ":", MakeStepInfoString(pJob->pNtk, cost, pJob->costInitial, pJob->duration));
        prt_.Insert(pJob->pNtk);
      });
    }
    if(par_.fOptOnInsert) {
      Cost costInitial = par_.fnObjective(pNtk_);
      Duration duration = 0;
      CallAbc(pNtk_, std::string("&put; ") + pCompress2rs + "; dc2; &get", duration);
      prt_.AssignNetwork(pNtk_);
      Cost cost = par_.fnObjective(pNtk_);
      Print(0, "", "c2rs; dc2", ":", std::string(8 + 3 * PrintFormat::int_width, ' '), MakeStepInfoString(pNtk_, cost, costInitial, duration));
    }
  }
  
  template <typename Ntk, typename Opt, typename Prt>
  void HeloScheduler<Ntk, Opt, Prt>::RunMultipleJobs() {
    Cost costBest = costStart_;
    for(int i = 0; i < par_.nJobs; i++) {
      Ntk *pCopy = new Ntk(*pNtk_);
      CreateJob(pCopy, par_.nSeed + i, costBest);
    }
    for(int i = 0; i < par_.nJobs; i++) {
      OnJobEnd([&](Job *pJob) {
        Cost cost = par_.fnObjective(pJob->pNtk);
        Print(0, "", "job", pJob->nId, "(", nFinishedJobs_ + 1, "/", par_.nJobs, ")", ":", MakeStepInfoString(pJob->pNtk, cost, pJob->costInitial, pJob->duration));
        if(cost < costBest) {
          costBest = cost;
          pNtk_->Read(*(pJob->pNtk));
        }
        delete pJob->pNtk;
      });
    }
  }  

  template <typename Ntk, typename Opt, typename Prt>
  void HeloScheduler<Ntk, Opt, Prt>::RunSingleJob() {
    CreateJob(pNtk_, par_.nSeed, costStart_);
    OnJobEnd([&](Job *pJob) {
      Cost cost = par_.fnObjective(pJob->pNtk);
      Print(0, "", "job", pJob->nId, "(", nFinishedJobs_ + 1, "/", par_.nJobs, ")", ":", MakeStepInfoString(pJob->pNtk, cost, pJob->costInitial, pJob->duration));
    });
  }
  
} // namespace boop

BOOP_HEADER_END
