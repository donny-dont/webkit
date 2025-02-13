/*
 * Copyright (C) 2015-2022 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "Exception.h"

#include "Interpreter.h"
#include "JSCJSValueInlines.h"
#include "JSObjectInlines.h"
#include "StructureInlines.h"
#include "JSWebAssemblyException.h"
#include "WasmTag.h"

namespace JSC {

const ClassInfo Exception::s_info = { "Exception"_s, nullptr, nullptr, nullptr, CREATE_METHOD_TABLE(Exception) };

Exception* Exception::create(VM& vm, JSValue thrownValue, StackCaptureAction action)
{
    Exception* result = new (NotNull, allocateCell<Exception>(vm)) Exception(vm, thrownValue);
    result->finishCreation(vm, action);
    return result;
}

void Exception::destroy(JSCell* cell)
{
    Exception* exception = static_cast<Exception*>(cell);
    exception->~Exception();
}

Structure* Exception::createStructure(VM& vm, JSGlobalObject* globalObject, JSValue prototype)
{
    return Structure::create(vm, globalObject, prototype, TypeInfo(CellType, StructureFlags), info());
}

template<typename Visitor>
void Exception::visitChildrenImpl(JSCell* cell, Visitor& visitor)
{
    Exception* thisObject = jsCast<Exception*>(cell);
    ASSERT_GC_OBJECT_INHERITS(thisObject, info());
    Base::visitChildren(thisObject, visitor);

    visitor.append(thisObject->m_value);
    for (StackFrame& frame : thisObject->m_stack)
        frame.visitAggregate(visitor);
    visitor.reportExtraMemoryVisited(thisObject->m_stack.sizeInBytes());
}

DEFINE_VISIT_CHILDREN(Exception);

Exception::Exception(VM& vm, JSValue thrownValue)
    : Base(vm, vm.exceptionStructure.get())
    , m_value(thrownValue, WriteBarrierEarlyInit)
{
}

Exception::~Exception() = default;

void Exception::finishCreation(VM& vm, StackCaptureAction action)
{
    Base::finishCreation(vm);

    Vector<StackFrame> stackTrace;
    if (action == StackCaptureAction::CaptureStack)
        vm.interpreter.getStackTrace(this, stackTrace, 0, Options::exceptionStackTraceLimit());
    m_stack = WTFMove(stackTrace);
    vm.heap.reportExtraMemoryAllocated(this, m_stack.sizeInBytes());
}

#if ENABLE(WEBASSEMBLY)

void Exception::wrapValueForJSTag(JSGlobalObject* globalObject)
{
    VM& vm = globalObject->vm();
    FixedVector<uint64_t> payload { static_cast<uint64_t>(JSValue::encode(m_value.get())) };
    auto* wrapper = JSWebAssemblyException::create(vm, globalObject->webAssemblyExceptionStructure(), Wasm::Tag::jsExceptionTag(), WTFMove(payload));
    m_value.set(vm, this, wrapper);
}

#endif

} // namespace JSC
