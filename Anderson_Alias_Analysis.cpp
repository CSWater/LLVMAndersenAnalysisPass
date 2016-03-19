#include "llvm/Pass.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"
#include <set>
#include "stdio.h"

using namespace llvm;

typedef std::map<std::string, std::set<std::string> > MapToSet;
typedef std::map<std::string, std::set<std::string> >::iterator MapToSetIterator;

typedef std::set<std::string> PtsSet;
typedef std::set<std::string>::iterator PtsSetIterator;

typedef std::map<Value *, std::string> MapTempValues2String;

typedef std::map<Value *, Value *> MapValue2Value;
typedef std::map<Value *, bool> MapArrayIndex;

class AndersenPA : public ModulePass {

    MapToSet PointsTo;
    MapTempValues2String TempValues;
    MapValue2Value GEPValueMap;
    MapArrayIndex CallReturnValues;
    MapTempValues2String ReturnValues;
    
    int tempVarCount;
    public:
    static char ID;
    AndersenPA() : ModulePass(ID) {}
    bool runOnModule(Module &M) {
//        errs() << "Andersen Alias Analysis: \n\n";
//        errs().write_escaped(M.getName()) << "\n";
//        M.dump();
        tempVarCount = 0;
        for (Module::global_iterator G = M.global_begin(), GE = M.global_end();
             G != GE; G++) {

//            errs() << "Global Variable: " << (G)->getName() << "\n";
            (PointsTo[G->getName()]).insert("");
        }

        bool LocalChange = true;
        while (LocalChange) {
            LocalChange = false;
            int i = 0;
            for (Module::iterator F = M.begin(), FE = M.end(); F != FE; F++) {

                LocalChange |= createInitialConstraints(*F);
//                errs() << "$$$$$$$$$Iteration : " << i++ << "Flag = " << LocalChange << "\n";
            }
        }
        for(MapToSetIterator I = PointsTo.begin(), E = PointsTo.end(); 
            I != E; I++) {

//            errs() << I->first << "PrintChange----->";
            StringRef a = I->first;
            std::pair<StringRef, StringRef> Split = a.split('.');
//            errs() << "\nChanged " << I->first << " to " << Split.first << "\n";
            if (!Split.second.empty()) {

//                errs() << "Change Needed";
            
                for(PtsSetIterator SI = I->second.begin(), SE = I->second.end(); 
                    SI != SE; SI++) {
                
                    if (*SI != Split.second) {
                        PointsTo[Split.second].insert(*SI);
                    }
                }
                PointsTo.erase(I->first);
            }
        }

        for(MapToSetIterator I = PointsTo.begin(), E = PointsTo.end(); 
            I != E; I++) {

            if(I->first == "")
                continue;
            outs() << I->first << " ";
            
            for(PtsSetIterator SI = I->second.begin(), SE = I->second.end(); 
                SI != SE; SI++) {
                
                outs() << *SI << "  ";
            }
            outs() << "\n";
        }
        outs() << "\n\n";
        return false;
    }

    bool createInitialConstraints(Function &F);

    std::string getTempName() {

        char OutString[20];
        sprintf(OutString, "%s%d", "@@ZXY@@_", tempVarCount++);
        //return (std::string("temp") + std::to_string(tempVarCount));
        //errs() << "New Temp value :" << OutString << "\n";
        return OutString;
    }
}; // end of class AndersenPA

bool AndersenPA::createInitialConstraints(Function &F) {

//    StringRef FuncName = F.getName();
    bool ChangeInPtsSet = false;

    for (Function::iterator B = F.begin(), BE = F.end(); B != BE; B++) {

        for (BasicBlock::iterator I = B->begin(), IE = B->end(); I != IE;
             I++) {

            switch(I->getOpcode()) {

                case Instruction::Alloca:
                  {
                    ;
                    //errs() << "Alloca : " << I->getName() << "\n";
                    (PointsTo[I->getName()]).insert("");
                  }
                  break;
                case Instruction::GetElementPtr:
                  {
                    GetElementPtrInst *GEPI = dyn_cast<GetElementPtrInst>(I);
                    Value *v = GEPI;
                    Value *ptr = GEPI->getPointerOperand();
                    
                    GEPValueMap[v] = ptr;

/*                    const Instruction *I = dyn_cast<Instruction>(ptr);
                    const AllocaInst *AI = dyn_cast<AllocaInst>(I);
                    if (AI->getAllocatedType()->isStructTy()) {

                        errs() << "Found Struct type\n";
                    }
                    else {
                        errs() << "NotStruct type" << AI->getNumOperands() << "\n";

                    }*/

                    /*std::string ptrName;
                    std::string valueName;
                    bool ptrIsTemp = false;
                    bool valIsTemp = false;
                    if (!v->hasName()) {
                        valIsTemp = true;
                        errs() << "Value Does not have a name\n";
                        if (TempValues[v].empty()) {

                            std::string temp = getTempName();
                            TempValues[v] = temp;
                            valueName = temp;
                        }
                        else{
                            valueName = TempValues[v];
                        }
                        //v->setName("temp");
                    }else {
                        errs() << "Value have a name\n";
                        valueName = v->getName();
                    }
                    if (!ptr->hasName()) {
                        ptrIsTemp = true;
                        errs() << "Ptr: Does not have a name\n";
                        if (TempValues[ptr].empty()) {

                            std::string temp = getTempName();
                            TempValues[ptr] = temp;
                            ptrName = temp;
                        }
                        else{
                            ptrName = TempValues[ptr];
                        }
                        //ptr->setName("temp");
                    }else {
                        errs() << "Ptr: Have a name\n";
                        ptrName = ptr->getName();
                    }
                    for (PtsSetIterator PI = PointsTo[ptrName].begin(),
                         PE = PointsTo[ptrName].end(); PI != PE; PI++) {
                        PointsTo[valueName].insert(*PI);
                    }*/
                  }
                  break;
                case Instruction::Store:
                  {
                    StoreInst *SI = dyn_cast<StoreInst>(I);
                    Value *v = SI->getValueOperand();
//                    errs() << "Store1 : " << v->getName() << "\n";
                    Value *ptr = SI->getPointerOperand();
                    //errs() << "Store : Val= " << v->getName() << "" << 
                    //          ptr->getName() << "\n";
                    bool valIsTemp = false, ptrIsTemp = false;

                    if ((!isa<GlobalVariable>(v) && isa<Constant>(v)) || 
                        (!isa<GlobalVariable>(ptr) && isa<Constant>(ptr))) {
//                        errs() << "constant\n";
                        break;
                    }

                    if(v->getType()->isPointerTy()) {
                        //errs() << "Value " << v->getName() << " is a pointer\n";
                        
                        if(ptr->getType()->isPointerTy())
                            ;//errs() << "Ptr:" << ptr->getName() << " is a pointer\n";

                        else
                            break;
                    }
                    else {
                        //errs() << "Break for Not Pointer type\n";
                        break;
                    }
                    
                    std::string valueName;
                    std::string ptrName;
                    if (!v->hasName()) {
                        valIsTemp = true;
                        //errs() << "Value Does not have a name\n";
                        if (TempValues[v].empty()) {

                            std::string temp = getTempName();
                            TempValues[v] = temp;
                            valueName = temp;
                        }
                        else{
                            valueName = TempValues[v];
                        }
                        //v->setName("temp");
                    }else {
                        //errs() << "Value have a name\n";
                        valueName = v->getName();
                    }
                    
                    if (!ptr->hasName()) {
                        ptrIsTemp = true;
                        //errs() << "Ptr: Does not have a name\n";
                        if (TempValues[ptr].empty()) {

                            std::string temp = getTempName();
                            TempValues[ptr] = temp;
                            ptrName = temp;
                        }
                        else{
                            ptrName = TempValues[ptr];
                        }
                        //ptr->setName("temp");
                    }else {
                        //errs() << "Ptr: Have a name\n";
                        ptrName = ptr->getName();
                    }

                    if (v->getValueID() == (Value::InstructionVal
                        + Instruction::GetElementPtr)) {

//                        errs() <<"*******Found element ptr value*******";
                        if (GEPValueMap[v] == NULL) {

                            errs() << "Could not find Value in GEPValueMap table";
                            abort();
                        }
                        v = GEPValueMap[v];
                        valueName = v->getName();
                    }
                    if (ptr->getValueID() == (Value::InstructionVal
                        + Instruction::GetElementPtr)) {

//                        errs() <<"*******Found element ptr value*******";
                        if (GEPValueMap[ptr] == NULL) {

                            errs() << "Could not find Value in GEPValueMap table";
                            abort();
                        }
                        ptr = GEPValueMap[ptr];
                        ptrName = ptr->getName();
                    }
                    //errs() << "Adding to : ";
                    if (!valIsTemp) {

                        if (!ptrIsTemp) {

                            if (!ReturnValues[v].empty()) {
                                valueName = ReturnValues[v];
                                //errs() << "CCCCCCCCCCCCCCCCCCCCCCCCCCC";
                                for (PtsSetIterator PI = PointsTo[valueName].begin(),
                                     PE = PointsTo[valueName].end(); PI != PE; PI++) {

                                    unsigned k = PointsTo[ptrName].size();
                                    PointsTo[ptrName].insert(*PI);
                                    if (!ChangeInPtsSet && k < PointsTo[ptrName].size())
                                        ChangeInPtsSet = true;

                                    //errs() << *PI << ", ";
                                }
                            } else {
                                unsigned k = PointsTo[ptrName].size();
                                PointsTo[ptrName].insert(valueName);
                                if (!ChangeInPtsSet && k < PointsTo[ptrName].size())
                                    ChangeInPtsSet = true;

                            }
                        }
                        else {

                            for (PtsSetIterator PI = PointsTo[ptrName].begin(),
                                 PE = PointsTo[ptrName].end(); PI != PE; PI++) {
                                unsigned k = PointsTo[*PI].size();
                                PointsTo[*PI].insert(valueName);
                                if (!ChangeInPtsSet && k < PointsTo[*PI].size())
                                    ChangeInPtsSet = true;

                                //errs() << *PI << ", ";
                            }
                        }
                    }
                    else {
                        if (!ptrIsTemp){

                            for (PtsSetIterator PI = PointsTo[valueName].begin(),
                                 PE = PointsTo[valueName].end(); PI != PE; PI++) {
                                unsigned k = PointsTo[ptrName].size();
                                PointsTo[ptrName].insert(*PI);
                                if (!ChangeInPtsSet && k < PointsTo[ptrName].size())
                                    ChangeInPtsSet = true;

                                //errs() << *PI << ", ";
                            }
                        }
                        else {

                            for (PtsSetIterator PI = PointsTo[ptrName].begin(),
                                 PE = PointsTo[ptrName].end(); PI != PE; PI++) {

                                for (PtsSetIterator PPI = PointsTo[valueName].begin(),
                                     PPE = PointsTo[valueName].end(); PPI != PPE; PPI++) {
                                    
                                    unsigned k = PointsTo[*PPI].size();
                                    PointsTo[*PI].insert(*PPI);
                                    if (!ChangeInPtsSet && k < PointsTo[*PPI].size())
                                        ChangeInPtsSet = true;

                                    //errs() << *PI << ", ";
                                }
                            }
                        }
                    }
                    /*for (PtsSetIterator PI = PointsTo[ptrName].begin(),
                         PE = PointsTo[ptrName].end(); PI != PE; PI++) {
                    
                        PointsTo[*PI].insert(valueName);
                        errs() << *PI << ", ";
                    }*/
                    //errs() << " points to set an additional element: " 
                    //       << valueName << "\n";
                  }
                  break;
                case Instruction::Load:
                  {
                    LoadInst *LI = dyn_cast<LoadInst>(I);
                    Value *v = LI;
//                    errs() << "Load1 : " << v->getName() << "\n";
                    Value *ptr = LI->getPointerOperand();
                    //errs() << "Load : Val= " << v->getName() << " ," 
                    //       << ptr->getName() << "\n";
                    bool valIsTemp = false, ptrIsTemp = false;

                    if ((!isa<GlobalVariable>(v) && isa<Constant>(v)) || 
                        (!isa<GlobalVariable>(ptr) && isa<Constant>(ptr))) {
//                        errs() << "constant\n";
                        break;
                    }

                    std::string valueName;
                    std::string ptrName;
                    
                    if(ptr->getType()->isPointerTy())
                        ;//errs() << "Ptr:" << ptr->getName() << " is a pointer\n";

                    else {
                        //errs() << "Break for Not Pointer type\n";
                        break;
                    }
                    /*}
                    else {
                        errs() << "Break for Not Pointer type\n";
                        break;
                    }*/

                    if (!v->hasName()) {
                        valIsTemp = true;
                        //errs() << "Value Does not have a name\n";
                        if (TempValues[v].empty()) {

                            std::string temp = getTempName();
                            TempValues[v] = temp;
                            valueName = temp;
                        }
                        else{
                            valueName = TempValues[v];
                        }
                    }else {
                        //errs() << "Value have a name\n";
                        valueName = v->getName();
                    }

                    if (!ptr->hasName()) {
                        ptrIsTemp = true;
                        //errs() << "Ptr: Does not have a name\n";
                        if (TempValues[ptr].empty()) {

                            std::string temp = getTempName();
                            TempValues[ptr] = temp;
                            ptrName = temp;
                        }
                        else{
                            ptrName = TempValues[ptr];
                        }
                        //ptr->setName("temp");
                    }else {
                        //errs() << "Ptr: Have a name\n";
                        ptrName = ptr->getName();
                    }
                    
                    if (ptr->getValueID() == (Value::InstructionVal
                        + Instruction::GetElementPtr)) {

//                        errs() <<"*******Found element ptr value*******";
                        if (GEPValueMap[ptr] == NULL) {

                            errs() << "Could not find Value in GEPValueMap table";
                            abort();
                        }
                        ptr = GEPValueMap[ptr];
                        ptrName = ptr->getName();
                    }

                    //errs() << "Adding to : ";

                    if (!ptrIsTemp) {

                        for (PtsSetIterator PI = PointsTo[ptrName].begin(),
                             PE = PointsTo[ptrName].end(); PI != PE; PI++) {
                            unsigned k = PointsTo[valueName].size();
                            PointsTo[valueName].insert(*PI);
                            if (!ChangeInPtsSet && k < PointsTo[valueName].size())
                                ChangeInPtsSet = true;

                        }
                    }else {

                    for (PtsSetIterator PI = PointsTo[ptrName].begin(),
                         PE = PointsTo[ptrName].end(); PI != PE; PI++) {
                    
                        for (PtsSetIterator PPI = PointsTo[*PI].begin(),
                             PPE = PointsTo[*PI].end(); PPI != PPE; PPI++) {
                            unsigned k = PointsTo[valueName].size();
                            PointsTo[valueName].insert(*PPI);
                            if (!ChangeInPtsSet && k < PointsTo[valueName].size())
                                ChangeInPtsSet = true;
                        }
                    }
                    //errs() << " points to set an additional element: " 
                    //       << v->getName() << "\n";
                    }
                  }
                break;
                case Instruction::Call:
                  {
                    CallInst *CI = dyn_cast<CallInst>(I);

                    Function *FuncCalled = CI->getCalledFunction();
                    Value *CalledValue = CI;
                    //errs() <<"999999 : " << CalledValue->getName() << "\n";
                    if (CalledValue->hasName()) {

                        std::string FuncName = F.getName();
                        std::string c = CalledValue->getName();
                        std::string callValueName = FuncName + c;
                        //errs() <<"((((((( : " << callValueName << "\n";
                        for (PtsSetIterator PI = PointsTo[FuncCalled->getName()].begin(),
                             PE = PointsTo[FuncCalled->getName()].end(); PI != PE; PI++) {
                            unsigned k = PointsTo[callValueName].size();
                            PointsTo[callValueName].insert(*PI);
                            if (!ChangeInPtsSet && k < PointsTo[callValueName].size())
                                ChangeInPtsSet = true;

                        }
                        ReturnValues[CalledValue] = callValueName;
                    }

                    int i = 0;
                    for (Function::arg_iterator FA = FuncCalled->arg_begin(),
                         FAE = FuncCalled->arg_end(); FA != FAE; FA++, i++) {
                        
                        Value *actualArg = CI->getArgOperand(i);
                        std::string formalArgName = FA->getName();
                        std::string actualArgName;
                        if (actualArg->hasName()) {
                            actualArgName = actualArg->getName();
                            PointsTo[formalArgName].insert(actualArgName);
                            continue;
                        }
                        else
                            actualArgName = TempValues[actualArg];
                        PtsSetIterator actualArgSetIt = 
                            PointsTo[actualArgName].begin();
                        PtsSetIterator actualArgSetItEnd = 
                            PointsTo[actualArgName].end();

                        while(actualArgSetIt != actualArgSetItEnd) {
//                            //errs() << "Addding to PTS Set of " << formalArgName << 
//                                    " pts set element " << *actualArgSetIt << "\88888888n";
                            unsigned k = PointsTo[formalArgName].size();
                            PointsTo[formalArgName].insert(*actualArgSetIt);
                            if (!ChangeInPtsSet && k < PointsTo[formalArgName].size())
                                ChangeInPtsSet = true;

                            actualArgSetIt++;
                        }
                    }
                    //errs() << "--------------------------\n";
                  }
                break;
                case Instruction::Ret:
                  {
                    
                    ReturnInst *RI = dyn_cast<ReturnInst>(I);
                    //errs() <<"2222222 : " << RI->getName() << "\n";
                    if (RI->getNumOperands() != 0) {
                        Value *retVal = RI->getOperand(0);
                        std::string returnName = retVal->getName();
                        if (!retVal->hasName()) {
                            returnName = TempValues[retVal];
                        }
                        //errs() <<"2222222 : " << returnName << "\n";
                        //errs() << F.getName();
                        //PointsTo[F.getName()].insert(*actualArgSetIt);
                        for (PtsSetIterator PI = PointsTo[returnName].begin(),
                             PE = PointsTo[returnName].end(); PI != PE; PI++) {
                            unsigned k = PointsTo[F.getName()].size();
                            PointsTo[F.getName()].insert(*PI);
                            if (!ChangeInPtsSet && k < PointsTo[F.getName()].size())
                                ChangeInPtsSet = true;

                        }
                    }
                  }
                break;
            }
//            I->dump();
            
        }
    }
//    errs() << FuncName;
    return ChangeInPtsSet;
}

char AndersenPA::ID = 0;


static RegisterPass<AndersenPA> X("andpa", "Andersen Points To Alias Analysis"
                            " pass",
                            false /* Only looks at CFG */,
                            false /* Analysis Pass */);

