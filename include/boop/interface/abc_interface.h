#pragma once

#include <string>

#include "boop/config.h"

#ifdef BOOP_USE_ABC

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <vector>

#include "boop/network/types.h"

#include "aig/gia/gia.h"
#include "base/main/main.h"
#include "base/cmd/cmd.h"
#include "proof/cec/cec.h"

BOOP_HEADER_START

namespace boop {

  namespace abc_interface {

    ABC_NAMESPACE_USING_NAMESPACE

    inline void StartAbc() {
      Abc_Start();
    }

    inline void StopAbc() {
      Abc_Stop();
    }

    inline void ExecuteCommand(Abc_Frame_t *pAbc, const std::string &strCommand) {
      if(Abc_FrameIsBatchMode()) {
        int r = Cmd_CommandExecute(pAbc, strCommand.c_str());
        assert(r == 0);
      } else {
        Abc_FrameSetBatchMode(1);
        int r = Cmd_CommandExecute(pAbc, strCommand.c_str());
        assert(r == 0);
        Abc_FrameSetBatchMode(0);
      }
    }

    template <typename Ntk>
    void GiaReader(Gia_Man_t *pGia, Ntk *pNtk) {
      int i;
      Gia_Obj_t *pObj;
      pNtk->Reserve(Gia_ManObjNum(pGia));
      Gia_ManConst0(pGia)->Value = pNtk->GetConst0();
      Gia_ManForEachObj1(pGia, pObj, i) {
        if(Gia_ObjIsCi(pObj)) {
          pObj->Value = pNtk->AddPi();
        } else if(Gia_ObjIsCo(pObj)) {
          pNtk->AddPo(Gia_ObjFanin0(pObj)->Value, Gia_ObjFaninC0(pObj));
        } else {
          // TODO: support XOR (and BUF and MUX?), maybe create another function
          pObj->Value = pNtk->AddAnd(Gia_ObjFanin0(pObj)->Value, Gia_ObjFanin1(pObj)->Value, Gia_ObjFaninC0(pObj), Gia_ObjFaninC1(pObj));
        }
      }
    }

    template <typename Ntk>
    Gia_Man_t *CreateGia(Ntk *pNtk, bool fHash = true) {
      Gia_Man_t *pGia = Gia_ManStart(pNtk->GetNumNodes());
      if(fHash) {
        Gia_ManHashStart(pGia);
      }
      std::vector<int> vLits(pNtk->GetNumNodes());
      vLits[0] = Gia_ManConst0Lit();
      pNtk->ForEachPi([&](int nId) {
        vLits[nId] = Gia_ManAppendCi(pGia);
      });
      pNtk->ForEachInt([&](int nId) {
         assert(pNtk->GetNodeType(nId) == AND);
         int nLit = -1;
         pNtk->ForEachFanin(nId, [&](int nFi, bool fCompl) {
           if(nLit == -1) {
             nLit = Abc_LitNotCond(vLits[nFi], fCompl);
           } else if(fHash) {
             nLit = Gia_ManHashAnd(pGia, nLit, Abc_LitNotCond(vLits[nFi], fCompl));
           } else {
             nLit = Gia_ManAppendAnd(pGia, nLit, Abc_LitNotCond(vLits[nFi], fCompl));
           }
         });
         if(nLit == -1) {
           nLit = Abc_LitNot(vLits[0]);
         }
         vLits[nId] = nLit;
      });
      pNtk->ForEachPoDriver([&](int nFi, bool fCompl) {
         Gia_ManAppendCo(pGia, Abc_LitNotCond(vLits[nFi], fCompl));
      });
      if(fHash) {
        Gia_ManHashStop(pGia);
      }
      return pGia;
    }

    template <typename Ntk>
    void DumpGia(const std::string &strFilename, Ntk *pNtk, bool fHash) {
      Gia_Man_t *pGia = CreateGia(pNtk, fHash);
      char *pFilename = static_cast<char *>(malloc(strFilename.size() + 1));
      strcpy(pFilename, strFilename.c_str());
      Gia_AigerWrite(pGia, pFilename, 0, 0, 0);
      free(pFilename);
      Gia_ManStop(pGia);
    }

    template <typename Ntk>
    void Abc9Execute(Ntk *pNtk, const std::string &strCommand) {
      Abc_Frame_t *pAbc = Abc_FrameGetGlobalFrame();
      Abc_FrameUpdateGia(pAbc, CreateGia(pNtk));
      ExecuteCommand(pAbc, strCommand);
      Gia_Man_t *pGia = Abc_FrameReadGia(pAbc);
      pNtk->Read(pGia, GiaReader<Ntk>);
    }

    inline void AbcLmsStart(const std::string &strLibName) {
      std::string strCommand = "rec_start3 " + strLibName;
      Abc_Frame_t *pAbc = Abc_FrameGetGlobalFrame();
      ExecuteCommand(pAbc, strCommand);
    }

    template <typename Ntk>
    bool AbcVerify(Ntk *pNtk, Ntk *pAnother) {
      Gia_Man_t *pGia = CreateGia(pNtk);
      Gia_Man_t *pNew = CreateGia(pAnother);
      bool r = Cec_ManVerifyTwo(pGia, pNew, 0);
      Gia_ManStop(pGia);
      Gia_ManStop(pNew);
      return r;
    }

  } // namespace abc_interface

  inline void StartAbc() {
    abc_interface::StartAbc();
  }

  inline void StopAbc() {
    abc_interface::StopAbc();
  }

  template <typename Ntk>
  inline void Abc9Execute(Ntk *pNtk, const std::string &strCommand) {
    abc_interface::Abc9Execute(pNtk, strCommand);
  }

} // namespace boop

BOOP_HEADER_END

#else

BOOP_HEADER_START

namespace boop {

  inline void StartAbc() {
  }

  inline void StopAbc() {
  }

  template <typename Ntk>
  inline void Abc9Execute(Ntk *pNtk, const std::string &strCommand) {
    (void)pNtk;
    (void)strCommand;
  }

} // namespace boop

BOOP_HEADER_END

#endif
