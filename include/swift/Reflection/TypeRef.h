//===--- TypeRef.h - Swift Type References for Reflection -------*- C++ -*-===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2016 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See http://swift.org/LICENSE.txt for license information
// See http://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//
//
// Implements the structures of type references for property and enum
// case reflection.
//
//===----------------------------------------------------------------------===//

#ifndef SWIFT_REFLECTION_TYPEREF_H
#define SWIFT_REFLECTION_TYPEREF_H

#include "swift/Basic/Demangle.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/Support/Casting.h"

#include <iostream>

class NodePointer;

namespace swift {
namespace reflection {

template <typename Runtime>
class ReflectionContext;

struct ReflectionInfo;

using llvm::cast;
using llvm::dyn_cast;

enum class TypeRefKind {
#define TYPEREF(Id, Parent) Id,
#include "swift/Reflection/TypeRefs.def"
#undef TYPEREF
};

class TypeRef;
using TypeRefPointer = std::shared_ptr<TypeRef>;
using ConstTypeRefPointer = std::shared_ptr<const TypeRef>;
using TypeRefVector = std::vector<TypeRefPointer>;
using ConstTypeRefVector = const std::vector<TypeRefPointer>;
using DepthAndIndex = std::pair<unsigned, unsigned>;
using GenericArgumentMap = llvm::DenseMap<DepthAndIndex, TypeRefPointer>;

class TypeRef : public std::enable_shared_from_this<TypeRef> {
  TypeRefKind Kind;

public:
  TypeRef(TypeRefKind Kind) : Kind(Kind) {}

  TypeRefKind getKind() const {
    return Kind;
  }

  void dump() const;
  void dump(std::ostream &OS, unsigned Indent = 0) const;

  bool isConcrete() const;

  template <typename Runtime>
  TypeRefPointer
  subst(ReflectionContext<Runtime> &RC, GenericArgumentMap Subs);

  GenericArgumentMap getSubstMap() const;

  static TypeRefPointer fromDemangleNode(Demangle::NodePointer Node);
};

class BuiltinTypeRef final : public TypeRef {
  std::string MangledName;

public:
  BuiltinTypeRef(const std::string &MangledName)
    : TypeRef(TypeRefKind::Builtin), MangledName(MangledName) {}

  static std::shared_ptr<BuiltinTypeRef>
  create(const std::string &MangledName) {
    return std::make_shared<BuiltinTypeRef>(MangledName);
  }

  const std::string &getMangledName() const {
    return MangledName;
  }

  static bool classof(const TypeRef *TR) {
    return TR->getKind() == TypeRefKind::Builtin;
  }
};

class NominalTypeTrait {
  std::string MangledName;
  TypeRefPointer Parent;

protected:
  NominalTypeTrait(const std::string &MangledName, TypeRefPointer Parent)
      : MangledName(MangledName), Parent(Parent) {}

public:
  const std::string &getMangledName() const {
    return MangledName;
  }

  bool isStruct() const;
  bool isEnum() const;
  bool isClass() const;

  ConstTypeRefPointer getParent() const {
    return Parent;
  }

  TypeRefPointer getParent() {
    return Parent;
  }

  void setParent(TypeRefPointer P) {
    Parent = P;
  }

  unsigned getDepth() const;
};

class NominalTypeRef final : public TypeRef, public NominalTypeTrait {
public:
  NominalTypeRef(const std::string &MangledName,
                 TypeRefPointer Parent = nullptr)
    : TypeRef(TypeRefKind::Nominal), NominalTypeTrait(MangledName, Parent) {}

  static std::shared_ptr<NominalTypeRef>
  create(const std::string &MangledName, TypeRefPointer Parent = nullptr) {
    return std::make_shared<NominalTypeRef>(MangledName, Parent);
  }

  static bool classof(const TypeRef *TR) {
    return TR->getKind() == TypeRefKind::Nominal;
  }
};

class BoundGenericTypeRef final : public TypeRef, public NominalTypeTrait {
  TypeRefVector GenericParams;

public:
  BoundGenericTypeRef(const std::string &MangledName,
                      TypeRefVector GenericParams,
                      TypeRefPointer Parent = nullptr)
    : TypeRef(TypeRefKind::BoundGeneric),
      NominalTypeTrait(MangledName, Parent),
      GenericParams(GenericParams) {}

  static std::shared_ptr<BoundGenericTypeRef>
  create(const std::string &MangledName, TypeRefVector GenericParams,
         TypeRefPointer Parent = nullptr) {
    return std::make_shared<BoundGenericTypeRef>(MangledName, GenericParams,
                                                 Parent);
  }

  const TypeRefVector &getGenericParams() const {
    return GenericParams;
  }

  static bool classof(const TypeRef *TR) {
    return TR->getKind() == TypeRefKind::BoundGeneric;
  }
};

class TupleTypeRef final : public TypeRef {
  TypeRefVector Elements;
  bool Variadic;

public:
  TupleTypeRef(TypeRefVector Elements, bool Variadic=false)
    : TypeRef(TypeRefKind::Tuple), Elements(Elements), Variadic(Variadic) {}

  static std::shared_ptr<TupleTypeRef> create(TypeRefVector Elements,
                                              bool Variadic=false) {
    return std::make_shared<TupleTypeRef>(Elements, Variadic);
  }

  TypeRefVector getElements() {
    return Elements;
  };

  ConstTypeRefVector getElements() const {
    return Elements;
  };

  bool isVariadic() const {
    return Variadic;
  }

  static bool classof(const TypeRef *TR) {
    return TR->getKind() == TypeRefKind::Tuple;
  }
};

class FunctionTypeRef final : public TypeRef {
  TypeRefVector Arguments;
  TypeRefPointer Result;

public:
  FunctionTypeRef(TypeRefVector Arguments, TypeRefPointer Result)
    : TypeRef(TypeRefKind::Function), Arguments(Arguments), Result(Result) {}

  static std::shared_ptr<FunctionTypeRef> create(TypeRefVector Arguments,
                                                 TypeRefPointer Result) {
    return std::make_shared<FunctionTypeRef>(Arguments, Result);
  }

  TypeRefVector getArguments() {
    return Arguments;
  };

  ConstTypeRefVector getArguments() const {
    return Arguments;
  };

  TypeRefPointer getResult() {
    return Result;
  }

  ConstTypeRefPointer getResult() const {
    return Result;
  }

  static bool classof(const TypeRef *TR) {
    return TR->getKind() == TypeRefKind::Function;
  }
};

class ProtocolTypeRef final : public TypeRef {
  std::string ModuleName;
  std::string Name;

public:
  ProtocolTypeRef(const std::string &ModuleName,
                  const std::string &Name)
    : TypeRef(TypeRefKind::Protocol), ModuleName(ModuleName), Name(Name) {}

  static std::shared_ptr<ProtocolTypeRef>
  create(const std::string &ModuleName, const std::string &Name) {
    return std::make_shared<ProtocolTypeRef>(ModuleName, Name);
  }

  const std::string &getName() const {
    return Name;
  }

  const std::string &getModuleName() const {
    return ModuleName;
  }

  static bool classof(const TypeRef *TR) {
    return TR->getKind() == TypeRefKind::Protocol;
  }

  bool operator==(const ProtocolTypeRef &Other) {
    return ModuleName.compare(Other.ModuleName) == 0 &&
           Name.compare(Other.Name) == 0;
  }
  bool operator!=(const ProtocolTypeRef &Other) {
    return !(*this == Other);
  }
};

class ProtocolCompositionTypeRef final : public TypeRef {
  TypeRefVector Protocols;

public:
  ProtocolCompositionTypeRef(TypeRefVector Protocols)
    : TypeRef(TypeRefKind::ProtocolComposition), Protocols(Protocols) {}

  static std::shared_ptr<ProtocolCompositionTypeRef>
  create(TypeRefVector Protocols) {
    return std::make_shared<ProtocolCompositionTypeRef>(Protocols);
  }

  TypeRefVector getProtocols() {
    return Protocols;
  }

  ConstTypeRefVector getProtocols() const {
    return Protocols;
  }

  static bool classof(const TypeRef *TR) {
    return TR->getKind() == TypeRefKind::ProtocolComposition;
  }
};

class MetatypeTypeRef final : public TypeRef {
  TypeRefPointer InstanceType;

public:
  MetatypeTypeRef(TypeRefPointer InstanceType)
    : TypeRef(TypeRefKind::Metatype), InstanceType(InstanceType) {}

  static std::shared_ptr<MetatypeTypeRef> create(TypeRefPointer InstanceType) {
    return std::make_shared<MetatypeTypeRef>(InstanceType);
  }

  TypeRefPointer getInstanceType() {
    return InstanceType;
  }

  ConstTypeRefPointer getInstanceType() const {
    return InstanceType;
  }

  static bool classof(const TypeRef *TR) {
    return TR->getKind() == TypeRefKind::Metatype;
  }
};

class ExistentialMetatypeTypeRef final : public TypeRef {
  TypeRefPointer InstanceType;

public:
  ExistentialMetatypeTypeRef(TypeRefPointer InstanceType)
    : TypeRef(TypeRefKind::ExistentialMetatype), InstanceType(InstanceType) {}
  static std::shared_ptr<ExistentialMetatypeTypeRef>
  create(TypeRefPointer InstanceType) {
    return std::make_shared<ExistentialMetatypeTypeRef>(InstanceType);
  }

  TypeRefPointer getInstanceType() {
    return InstanceType;
  }

  ConstTypeRefPointer getInstanceType() const {
    return InstanceType;
  }

  static bool classof(const TypeRef *TR) {
    return TR->getKind() == TypeRefKind::ExistentialMetatype;
  }
};

class GenericTypeParameterTypeRef final : public TypeRef {
  const uint32_t Depth;
  const uint32_t Index;

public:
  GenericTypeParameterTypeRef(uint32_t Depth, uint32_t Index)
    : TypeRef(TypeRefKind::GenericTypeParameter), Depth(Depth), Index(Index) {}

  static std::shared_ptr<GenericTypeParameterTypeRef>
  create(uint32_t Depth, uint32_t Index) {
    return std::make_shared<GenericTypeParameterTypeRef>(Depth, Index);
  }

  uint32_t getDepth() const {
    return Depth;
  }

  uint32_t getIndex() const {
    return Index;
  }

  static bool classof(const TypeRef *TR) {
    return TR->getKind() == TypeRefKind::GenericTypeParameter;
  }
};

class DependentMemberTypeRef final : public TypeRef {
  std::string Member;
  TypeRefPointer Base;
  TypeRefPointer Protocol;

public:
  DependentMemberTypeRef(const std::string &Member, TypeRefPointer Base,
                         TypeRefPointer Protocol)
    : TypeRef(TypeRefKind::DependentMember), Member(Member), Base(Base),
      Protocol(Protocol) {}

  static std::shared_ptr<DependentMemberTypeRef>
  create(const std::string &Member, TypeRefPointer Base,
         TypeRefPointer Protocol) {
    return std::make_shared<DependentMemberTypeRef>(Member, Base, Protocol);
  }

  const std::string &getMember() const {
    return Member;
  }

  TypeRefPointer getBase() {
    return Base;
  }

  ConstTypeRefPointer getBase() const {
    return Base;
  }

  const ProtocolTypeRef *getProtocol() const {
    return cast<ProtocolTypeRef>(Protocol.get());
  }

  static bool classof(const TypeRef *TR) {
    return TR->getKind() == TypeRefKind::DependentMember;
  }
};

class ForeignClassTypeRef final : public TypeRef {
  std::string Name;

public:
  ForeignClassTypeRef(const std::string &Name)
    : TypeRef(TypeRefKind::ForeignClass), Name(Name) {}

  static const std::shared_ptr<ForeignClassTypeRef> Unnamed;

  static std::shared_ptr<ForeignClassTypeRef> create(const std::string &Name) {
    return std::make_shared<ForeignClassTypeRef>(Name);
  }

  const std::string &getName() const {
    return Name;
  }

  static bool classof(const TypeRef *TR) {
    return TR->getKind() == TypeRefKind::ForeignClass;
  }
};

class ObjCClassTypeRef final : public TypeRef {
  std::string Name;

public:
  ObjCClassTypeRef(const std::string &Name)
    : TypeRef(TypeRefKind::ObjCClass), Name(Name) {}

  static const std::shared_ptr<ObjCClassTypeRef> Unnamed;

  static std::shared_ptr<ObjCClassTypeRef> create(const std::string &Name) {
    return std::make_shared<ObjCClassTypeRef>(Name);
  }

  const std::string &getName() const {
    return Name;
  }

  static bool classof(const TypeRef *TR) {
    return TR->getKind() == TypeRefKind::ObjCClass;
  }
};

class OpaqueTypeRef final : public TypeRef {
public:
  OpaqueTypeRef() : TypeRef(TypeRefKind::Opaque) {}
  static const std::shared_ptr<OpaqueTypeRef> Opaque;
  static bool classof(const TypeRef *TR) {
    return TR->getKind() == TypeRefKind::Opaque;
  }
};

class ReferenceStorageTypeRef : public TypeRef {
  TypeRefPointer Type;

protected:
  ReferenceStorageTypeRef(TypeRefKind Kind, TypeRefPointer Type)
    : TypeRef(Kind), Type(Type) {}

public:
  ConstTypeRefPointer getType() const {
    return Type;
  }

  TypeRefPointer getType() {
    return Type;
  }

  void setType(TypeRefPointer T) {
    Type = T;
  }
};

class UnownedStorageTypeRef final : public ReferenceStorageTypeRef {
public:
  UnownedStorageTypeRef(TypeRefPointer Type)
    : ReferenceStorageTypeRef(TypeRefKind::UnownedStorage, Type) {}

  static std::shared_ptr<UnownedStorageTypeRef>
  create(TypeRefPointer Type) {
    return std::make_shared<UnownedStorageTypeRef>(Type);
  }

  static bool classof(const TypeRef *TR) {
    return TR->getKind() == TypeRefKind::UnownedStorage;
  }
};

class WeakStorageTypeRef final : public ReferenceStorageTypeRef {
public:
  WeakStorageTypeRef(TypeRefPointer Type)
    : ReferenceStorageTypeRef(TypeRefKind::WeakStorage, Type) {}

  static std::shared_ptr<WeakStorageTypeRef>
  create(TypeRefPointer Type) {
    return std::make_shared<WeakStorageTypeRef>(Type);
  }

  static bool classof(const TypeRef *TR) {
    return TR->getKind() == TypeRefKind::WeakStorage;
  }
};

class UnmanagedStorageTypeRef final : public ReferenceStorageTypeRef {
public:
  UnmanagedStorageTypeRef(TypeRefPointer Type)
    : ReferenceStorageTypeRef(TypeRefKind::UnmanagedStorage, Type) {}

  static std::shared_ptr<UnmanagedStorageTypeRef>
  create(TypeRefPointer Type) {
    return std::make_shared<UnmanagedStorageTypeRef>(Type);
  }

  static bool classof(const TypeRef *TR) {
    return TR->getKind() == TypeRefKind::UnmanagedStorage;
  }
};

template <typename ImplClass, typename RetTy = void, typename... Args>
class TypeRefVisitor {
public:

  RetTy visit(const TypeRef *typeRef, Args... args) {
    switch (typeRef->getKind()) {
#define TYPEREF(Id, Parent) \
    case TypeRefKind::Id: \
      return static_cast<ImplClass*>(this) \
        ->visit##Id##TypeRef(cast<Id##TypeRef>(typeRef), \
                           ::std::forward<Args>(args)...);
#include "swift/Reflection/TypeRefs.def"
    }
  }
};

template <typename Runtime>
class TypeRefSubstitution
  : public TypeRefVisitor<TypeRefSubstitution<Runtime>, TypeRefPointer> {
  using StoredPointer = typename Runtime::StoredPointer;
  ReflectionContext<Runtime> &RC;
  GenericArgumentMap Substitutions;
public:
  using TypeRefVisitor<TypeRefSubstitution<Runtime>, TypeRefPointer>::visit;
  TypeRefSubstitution(ReflectionContext<Runtime> &RC,
                      GenericArgumentMap Substitutions)
  : RC(RC),
    Substitutions(Substitutions) {}

  TypeRefPointer visitBuiltinTypeRef(const BuiltinTypeRef *B) {
    return std::make_shared<BuiltinTypeRef>(*B);
  }

  TypeRefPointer visitNominalTypeRef(const NominalTypeRef *N) {
    return std::make_shared<NominalTypeRef>(*N);
  }

  TypeRefPointer visitBoundGenericTypeRef(const BoundGenericTypeRef *BG) {
    TypeRefVector GenericParams;
    for (auto Param : BG->getGenericParams())
      GenericParams.push_back(visit(Param.get()));
    return std::make_shared<BoundGenericTypeRef>(BG->getMangledName(),
                                                 GenericParams);
  }

  TypeRefPointer visitTupleTypeRef(const TupleTypeRef *T) {
    TypeRefVector Elements;
    for (auto Element : T->getElements()) {
      Elements.push_back(visit(Element.get()));
    }
    return std::make_shared<TupleTypeRef>(Elements, T->isVariadic());
  }

  TypeRefPointer visitFunctionTypeRef(const FunctionTypeRef *F) {
    TypeRefVector SubstitutedArguments;
    for (auto Argument : F->getArguments())
      SubstitutedArguments.push_back(visit(Argument.get()));

    auto SubstitutedResult = visit(F->getResult().get());

    return std::make_shared<FunctionTypeRef>(SubstitutedArguments,
                                             SubstitutedResult);
  }

  TypeRefPointer visitProtocolTypeRef(const ProtocolTypeRef *P) {
    return std::make_shared<ProtocolTypeRef>(*P);
  }

  TypeRefPointer
  visitProtocolCompositionTypeRef(const ProtocolCompositionTypeRef *PC) {
    return std::make_shared<ProtocolCompositionTypeRef>(*PC);
  }

  TypeRefPointer visitMetatypeTypeRef(const MetatypeTypeRef *M) {
    return MetatypeTypeRef::create(visit(M->getInstanceType().get()));
  }

  TypeRefPointer
  visitExistentialMetatypeTypeRef(const ExistentialMetatypeTypeRef *EM) {
    assert(EM->getInstanceType()->isConcrete());
    return std::make_shared<ExistentialMetatypeTypeRef>(*EM);
  }

  TypeRefPointer
  visitGenericTypeParameterTypeRef(const GenericTypeParameterTypeRef *GTP) {
    auto found = Substitutions.find({GTP->getDepth(), GTP->getIndex()});
    assert(found != Substitutions.end());
    assert(found->second->isConcrete());
    return found->second;
  }

  TypeRefPointer
  visitDependentMemberTypeRef(const DependentMemberTypeRef *DM) {
    auto SubstBase = visit(DM->getBase().get());

    TypeRefPointer TypeWitness;

    switch (SubstBase->getKind()) {
    case TypeRefKind::Nominal: {
      auto Nominal = cast<NominalTypeRef>(SubstBase.get());
      TypeWitness = RC.getDependentMemberTypeRef(Nominal->getMangledName(), DM);
      break;
    }
    case TypeRefKind::BoundGeneric: {
      auto BG = cast<BoundGenericTypeRef>(SubstBase.get());
      TypeWitness = RC.getDependentMemberTypeRef(BG->getMangledName(), DM);
      break;
    }
    default:
      assert(false && "Unknown base type");
    }

    assert(TypeWitness);
    return TypeWitness->subst(RC, SubstBase->getSubstMap());
  }

  TypeRefPointer visitForeignClassTypeRef(const ForeignClassTypeRef *F) {
    return std::make_shared<ForeignClassTypeRef>(*F);
  }

  TypeRefPointer visitObjCClassTypeRef(const ObjCClassTypeRef *OC) {
    return std::make_shared<ObjCClassTypeRef>(*OC);
  }

  TypeRefPointer visitUnownedStorageTypeRef(const UnownedStorageTypeRef *US) {
    return UnownedStorageTypeRef::create(visit(US->getType().get()));
  }

  TypeRefPointer visitWeakStorageTypeRef(const WeakStorageTypeRef *WS) {
    return WeakStorageTypeRef::create(visit(WS->getType().get()));
  }

  TypeRefPointer
  visitUnmanagedStorageTypeRef(const UnmanagedStorageTypeRef *US) {
    return UnmanagedStorageTypeRef::create(visit(US->getType().get()));
  }

  TypeRefPointer visitOpaqueTypeRef(const OpaqueTypeRef *Op) {
    return std::make_shared<OpaqueTypeRef>(*Op);
  }
};

template <typename Runtime>
TypeRefPointer
TypeRef::subst(ReflectionContext<Runtime> &RC, GenericArgumentMap Subs) {
  TypeRefPointer Result = TypeRefSubstitution<Runtime>(RC, Subs).visit(this);
  assert(Result->isConcrete());
  return Result;
}

} // end namespace reflection
} // end namespace swift

#endif // SWIFT_REFLECTION_TYPEREF_H
