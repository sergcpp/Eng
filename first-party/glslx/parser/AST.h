#pragma once

#include <cstdlib>

#include <vector>

#include "../Bitmask.h"
#include "Lexer.h"

namespace glslx {
template <typename T> static inline void ast_destroy(void *self) {
    ((T *)self)->~T();
    free(self);
}

struct ast_node_base {
    int gc = 0;
};

struct ast_memory {
    ast_node_base *data = nullptr;
    void (*dtor)(void *) = nullptr;

    ast_memory() noexcept = default;
    template <typename T> ast_memory(T *_data) : data(_data), dtor(&ast_destroy<T>) {}
    void destroy() { dtor(data); }
};

template <typename T> struct ast_node : public ast_node_base {
    void *operator new(const size_t size, std::vector<ast_memory> *collector) noexcept {
        void *data = malloc(size);
        if (data) {
            collector->push_back(ast_memory{(T *)data});
        }
        return data;
    }
    void operator delete(void *, std::vector<ast_memory> *) {}

  private:
    void *operator new(size_t);
    void operator delete(void *) {}
};

// https://registry.khronos.org/OpenGL/specs/gl/GLSLangSpec.4.60.html#storage-qualifiers
enum class eStorage {
    None = -1, // local read/write memory, or an input parameter to a function
    Const,     // a variable whose value cannot be changed
    In,        // linkage into a shader from a previous stage, variable is copied in
    Out,       // linkage out of a shader to a subsequent stage, variable is copied out
    Attribute, // compatibility profile only and vertex language only; same as in when in a vertex shader
    Uniform, // value does not change across the primitive being processed, uniforms form the linkage between a shader,
             // API, and the application
    Varying, // compatibility profile only and vertex and fragment languages only; same as out when in a vertex shader
             // and same as in when in a fragment shader
    Buffer,  // value is stored in a buffer object, and can be read or written both by shader invocations and the API
    Shared,  // compute shader only; variable storage is shared across all work items in a workgroup
    RayPayload, // Ray generation, closest-hit, and miss shader only. Storage that becomes associated with the incoming
                // ray payload when it's location layout qualifier is passed as an input to traceRayEXT()
    RayPayloadIn, // Closest-hit, any-hit and miss shader only. Storage associated with the incoming ray payload that
                  // can be read to and written in the stages invoked by traceRayEXT()
    HitAttribute, // Intersection, any-hit, and closest-hit shader only. Storage associated with attributes of geometry
                  // intersected by a ray
    CallableData, // Ray generation, closest-hit, miss and callable shader only. Storage that becomes associated with
                  // the incoming callable data when it's location layout qualifier is passed as an input to
                  // executeCallableEXT()
    CallableDataIn, // Callable shader only. Storage associated with the incoming callable data can be read to or
                    // written in the callable stage
};

enum class eAuxStorage {
    None = -1,
    Centroid, // centroid-based interpolation
    Sample,   // per-sample interpolation
    Patch     // per-tessellation-patch attributes
};

// https://registry.khronos.org/OpenGL/specs/gl/GLSLangSpec.4.60.html#interpolation-qualifiers
enum class eInterpolation {
    None = -1,
    Smooth,       // perspective correct interpolation
    Flat,         // no interpolation
    Noperspective // linear interpolation
};

// https://registry.khronos.org/OpenGL/specs/gl/GLSLangSpec.4.60.html#precision-and-precision-qualifiers
enum class ePrecision {
    None = -1,
    Highp,   // 32-bit two�s complement for integers, 32-bit IEEE 754 floating-point for float
    Mediump, // SPIR-V RelaxedPrecision when targeting Vulkan, otherwise none.
    Lowp     // SPIR-V RelaxedPrecision when targeting Vulkan, otherwise none.
};

// https://registry.khronos.org/OpenGL/specs/gl/GLSLangSpec.4.60.html#memory-qualifiers
enum class eMemory : uint32_t {
    Coherent, // memory variable where reads and writes are coherent with reads and writes from other shader invocations
    Volatile, // memory variable whose underlying value may be changed at any point during shader execution by some
              // source other than the current shader invocation
    Restrict, // memory variable where use of that variable is the only way to read and write the underlying memory in
              // the relevant shader stage
    Readonly, // memory variable that can be used to read the underlying memory, but cannot be used to write the
              // underlying memory
    Writeonly // memory variable that can be used to write the underlying memory, but cannot be used to read the
              // underlying memory
};

enum class eParamQualifier {
    None,
    Const, // for function parameters that cannot be written to
    In,    // for function parameters passed into a function
    Out,   // for function parameters passed back out of a function, but not initialized for use when passed in
    Inout  // for function parameters passed both into and out of a function
};

enum class eCtrlFlowAttribute {
    // loop attributes
    Unroll,
    DontUnroll,
    Loop = DontUnroll,
    DependencyInfinite,
    DependencyLength,
    MinIterations,     // the loop executes at least a given number of iterations
    MaxIterations,     // the loop executes at most a given number of iterations
    IterationMultiple, // the loop executes a multiple of a given number of iterations
    PeelCount,         // the loop should be peeled by a given number of loop iterations
    PartialCount,      // the loop should be partially unrolled by a given number of loop iterations
    // selection attributes
    Flatten,
    DontFlatten,
    Branch = DontFlatten
};

enum class eFunctionAttribute { Builtin };

const Bitmask<eCtrlFlowAttribute> LoopAttributesMask =
    Bitmask<eCtrlFlowAttribute>{eCtrlFlowAttribute::Unroll} | eCtrlFlowAttribute::DontUnroll |
    eCtrlFlowAttribute::DependencyInfinite | eCtrlFlowAttribute::DependencyLength;
const Bitmask<eCtrlFlowAttribute> SelectionAttributesMask =
    Bitmask<eCtrlFlowAttribute>{eCtrlFlowAttribute::Flatten} | eCtrlFlowAttribute::DontFlatten;

struct ast_function;
struct ast_global_variable;
struct ast_version_directive;
struct ast_extension_directive;
struct ast_expression;
using ast_constant_expression = ast_expression;
struct ast_layout_qualifier;
struct ast_statement;
struct ast_struct;
struct ast_interface_block;
struct ast_variable;

enum class eTrUnitType {
    Compute,
    Vertex,
    TessControl,
    TessEvaluation,
    Geometry,
    Fragment,
    RayGen,
    Intersect,
    ClosestHit,
    AnyHit,
    Miss,
    Callable
};

struct TrUnit {
    eTrUnitType type;

    ast_version_directive *version = nullptr;
    std::vector<ast_extension_directive *> extensions;
    std::vector<ast_interface_block *> interface_blocks;
    std::vector<ast_struct *> structures;
    std::vector<ast_global_variable *> globals;
    std::vector<ast_function *> functions;

    std::vector<ast_memory> mem;
    std::vector<char *> str;

    TrUnit(eTrUnitType _type) : type(_type) {}
    ~TrUnit();
};

struct ast_type : ast_node<ast_type> {
    bool builtin;

    ast_type(const bool _builtin) noexcept : builtin(_builtin) {}
};

struct ast_builtin : ast_type {
    eKeyword type;

    ast_builtin(eKeyword _type) noexcept : ast_type(true), type(_type) {}
};

struct ast_struct : ast_type {
    const char *name = nullptr;
    std::vector<ast_variable *> fields;

    ast_struct() noexcept : ast_type(false) {}
};

struct ast_interface_block : ast_type {
    const char *name = nullptr;
    eStorage storage = eStorage::None;
    Bitmask<eMemory> memory_flags;
    std::vector<ast_variable *> fields;
    std::vector<ast_layout_qualifier *> layout_qualifiers;

    ast_interface_block() noexcept : ast_type(false) {}
};

struct ast_version_directive : ast_type {
    int number = -1;
    eVerType type = eVerType::Core;

    ast_version_directive() noexcept : ast_type(false) {}
};

struct ast_extension_directive : ast_type {
    char *name = nullptr;
    eExtBehavior behavior = eExtBehavior::Invalid;

    ast_extension_directive() noexcept : ast_type(false) {}
};

enum class eVariableType { Function, Parameter, Global, Field };

struct ast_variable : ast_node<ast_variable> {
    const char *name = nullptr;
    ast_type *base_type = nullptr;
    bool is_array = false;
    bool is_precise = false;
    eVariableType type;
    std::vector<ast_constant_expression *> array_sizes;

    ast_variable(eVariableType _type) noexcept : type(_type) {}
};

struct ast_function_variable : ast_variable {
    ast_expression *initial_value = nullptr;
    bool is_const = false;

    ast_function_variable() : ast_variable(eVariableType::Function) {}
};

struct ast_function_parameter : ast_variable {
    Bitmask<eParamQualifier> qualifiers = eParamQualifier::None;
    ePrecision precision = ePrecision::None;

    ast_function_parameter() noexcept : ast_variable(eVariableType::Parameter) {}
};

struct ast_global_variable : ast_variable {
    eStorage storage = eStorage::None;
    eAuxStorage aux_storage = eAuxStorage::None;
    Bitmask<eMemory> memory_flags;
    ePrecision precision = ePrecision::None;
    eInterpolation interpolation = eInterpolation::None;
    bool is_invariant = false;
    bool is_constant = false;
    bool is_hidden = false;
    ast_constant_expression *initial_value = nullptr;
    std::vector<ast_layout_qualifier *> layout_qualifiers;

    ast_global_variable() noexcept : ast_variable(eVariableType::Global) {}
};

struct ast_layout_qualifier : ast_node<ast_layout_qualifier> {
    const char *name = nullptr;
    ast_constant_expression *initial_value = nullptr;
};

struct ast_function : ast_node<ast_function> {
    ast_type *return_type = nullptr;
    const char *name = nullptr;
    std::vector<ast_function_parameter *> parameters;
    std::vector<ast_statement *> statements;
    bool is_prototype = false;
    ast_function *prototype = nullptr;
    Bitmask<eFunctionAttribute> attributes;
};

enum class eStatement {
    Compound,
    Empty,
    Declaration,
    Expression,
    If,
    Switch,
    CaseLabel,
    While,
    Do,
    For,
    Continue,
    Break,
    Return,
    Discard,
    _Count
};

struct ast_statement : ast_node<ast_statement> {
    eStatement type;

    ast_statement(eStatement _type) noexcept : type(_type) {}
};

struct ast_simple_statement : ast_statement {
    ast_simple_statement(eStatement _type) noexcept : ast_statement(_type) {}
};

struct ast_compound_statement : ast_statement {
    std::vector<ast_statement *> statements;
    ast_compound_statement() noexcept : ast_statement(eStatement::Compound) {}
};

struct ast_empty_statement : ast_simple_statement {
    ast_empty_statement() noexcept : ast_simple_statement(eStatement::Empty) {}
};

struct ast_declaration_statement : ast_simple_statement {
    std::vector<ast_function_variable *> variables;

    ast_declaration_statement() noexcept : ast_simple_statement(eStatement::Declaration) {}
};

struct ast_expression_statement : ast_simple_statement {
    ast_expression *expression = nullptr;

    ast_expression_statement(ast_expression *_expression) noexcept
        : ast_simple_statement(eStatement::Expression), expression(_expression) {}
};

struct ast_if_statement : ast_simple_statement {
    ast_expression *condition = nullptr;
    ast_statement *then_statement = nullptr;
    ast_statement *else_statement = nullptr;
    Bitmask<eCtrlFlowAttribute> attributes;

    ast_if_statement(Bitmask<eCtrlFlowAttribute> _attributes) noexcept
        : ast_simple_statement(eStatement::If), attributes(_attributes) {}
};

struct ast_switch_statement : ast_simple_statement {
    ast_expression *expression = nullptr;
    std::vector<ast_statement *> statements;
    Bitmask<eCtrlFlowAttribute> attributes;

    ast_switch_statement(Bitmask<eCtrlFlowAttribute> _attributes) noexcept
        : ast_simple_statement(eStatement::Switch), attributes(_attributes) {}
};

struct ast_case_label_statement : ast_simple_statement {
    ast_constant_expression *condition = nullptr;
    bool is_default = false;

    ast_case_label_statement() noexcept : ast_simple_statement(eStatement::CaseLabel) {}
};

struct ctrl_flow_params_t {
    Bitmask<eCtrlFlowAttribute> attributes;
    int dependency_length = 0;
    int min_iterations = 0, max_iterations = 0;
    int iteration_multiple = 0;
    int peel_count = 0, partial_count = 0;
};

struct ast_loop_statement : ast_simple_statement {
    ctrl_flow_params_t flow_params;

    ast_loop_statement(eStatement _type, ctrl_flow_params_t _flow_params) noexcept
        : ast_simple_statement(_type), flow_params(_flow_params) {}
};

struct ast_while_statement : ast_loop_statement {
    ast_simple_statement *condition = nullptr;
    ast_statement *body = nullptr;

    ast_while_statement(ctrl_flow_params_t _flow_params) noexcept
        : ast_loop_statement(eStatement::While, _flow_params) {}
};

struct ast_do_statement : ast_loop_statement {
    ast_statement *body = nullptr;
    ast_expression *condition = nullptr;

    ast_do_statement(ctrl_flow_params_t _flow_params) noexcept : ast_loop_statement(eStatement::Do, _flow_params) {}
};

struct ast_for_statement : ast_loop_statement {
    ast_simple_statement *init = nullptr;
    ast_expression *condition = nullptr;
    ast_expression *loop = nullptr;
    ast_statement *body = nullptr;

    ast_for_statement(ctrl_flow_params_t _flow_params) noexcept : ast_loop_statement(eStatement::For, _flow_params) {}
};

struct ast_jump_statement : ast_statement {
    ast_jump_statement(eStatement _type) noexcept : ast_statement(_type) {}
};

struct ast_continue_statement : ast_jump_statement {
    ast_continue_statement() noexcept : ast_jump_statement(eStatement::Continue) {}
};

struct ast_break_statement : ast_jump_statement {
    ast_break_statement() noexcept : ast_jump_statement(eStatement::Break) {}
};

struct ast_return_statement : ast_jump_statement {
    ast_expression *expression = nullptr;

    ast_return_statement() noexcept : ast_jump_statement(eStatement::Return) {}
};

struct ast_discard_statement : ast_jump_statement {
    ast_discard_statement() noexcept : ast_jump_statement(eStatement::Discard) {}
};

enum class eExprType {
    IntConstant,
    UIntConstant,
    FloatConstant,
    DoubleConstant,
    BoolConstant,
    VariableIdentifier,
    FieldOrSwizzle,
    ArraySubscript,
    FunctionCall,
    ConstructorCall,
    PostIncrement,
    PostDecrement,
    UnaryMinus,
    UnaryPlus,
    BitNot,
    LogicalNot,
    PrefixIncrement,
    PrefixDecrement,
    Sequence,
    Assign,
    Operation,
    Ternary,
    ArraySpecifier
};

struct ast_expression : ast_node<ast_expression> {
    eExprType type;

    ast_expression(eExprType _type) noexcept : type(_type) {}
};

struct ast_int_constant : ast_expression {
    int32_t value;

    ast_int_constant(int32_t _value) noexcept : ast_expression(eExprType::IntConstant), value(_value) {}
};

struct ast_uint_constant : ast_expression {
    uint32_t value;

    ast_uint_constant(uint32_t _value) noexcept : ast_expression(eExprType::UIntConstant), value(_value) {}
};

struct ast_float_constant : ast_expression {
    float value;

    ast_float_constant(float _value) noexcept : ast_expression(eExprType::FloatConstant), value(_value) {}
};

struct ast_double_constant : ast_expression {
    double value;

    ast_double_constant(double _value) noexcept : ast_expression(eExprType::DoubleConstant), value(_value) {}
};

struct ast_bool_constant : ast_expression {
    bool value;

    ast_bool_constant(bool _value) noexcept : ast_expression(eExprType::BoolConstant), value(_value) {}
};

struct ast_array_specifier : ast_expression {
    std::vector<ast_expression *> expressions;

    ast_array_specifier() noexcept : ast_expression(eExprType::ArraySpecifier) {}
};

struct ast_variable_identifier : ast_expression {
    ast_variable *variable;

    ast_variable_identifier(ast_variable *_variable) noexcept
        : ast_expression(eExprType::VariableIdentifier), variable(_variable) {}
};

struct ast_field_or_swizzle : ast_expression {
    ast_expression *operand = nullptr;
    ast_variable_identifier *field = nullptr;
    const char *name = nullptr;

    ast_field_or_swizzle() noexcept : ast_expression(eExprType::FieldOrSwizzle) {}
};

struct ast_array_subscript : ast_expression {
    ast_expression *operand = nullptr, *index = nullptr;

    ast_array_subscript() noexcept : ast_expression(eExprType::ArraySubscript) {}
};

struct ast_function_call : ast_expression {
    const char *name = nullptr;
    ast_function *func = nullptr;
    std::vector<ast_expression *> parameters;

    ast_function_call() noexcept : ast_expression(eExprType::FunctionCall) {}
};

struct ast_constructor_call : ast_expression {
    ast_type *type = nullptr;
    std::vector<ast_expression *> parameters;

    ast_constructor_call() noexcept : ast_expression(eExprType::ConstructorCall) {}
};

struct ast_unary_expression : ast_expression {
    ast_expression *operand;

    ast_unary_expression(eExprType _type, ast_expression *_operand) noexcept
        : ast_expression(_type), operand(_operand) {}
};

struct ast_binary_expression : ast_expression {
    ast_expression *operand1 = nullptr, *operand2 = nullptr;

    ast_binary_expression(eExprType _type) noexcept : ast_expression(_type) {}
};

struct ast_post_increment_expression : ast_unary_expression {
    ast_post_increment_expression(ast_expression *operand) noexcept
        : ast_unary_expression(eExprType::PostIncrement, operand) {}
};

struct ast_post_decrement_expression : ast_unary_expression {
    ast_post_decrement_expression(ast_expression *operand) noexcept
        : ast_unary_expression(eExprType::PostDecrement, operand) {}
};

struct ast_unary_plus_expression : ast_unary_expression {
    ast_unary_plus_expression(ast_expression *operand) noexcept : ast_unary_expression(eExprType::UnaryPlus, operand) {}
};

struct ast_unary_minus_expression : ast_unary_expression {
    ast_unary_minus_expression(ast_expression *operand) noexcept
        : ast_unary_expression(eExprType::UnaryMinus, operand) {}
};

struct ast_unary_bit_not_expression : ast_unary_expression {
    ast_unary_bit_not_expression(ast_expression *operand) noexcept : ast_unary_expression(eExprType::BitNot, operand) {}
};

struct ast_unary_logical_not_expression : ast_unary_expression {
    ast_unary_logical_not_expression(ast_expression *operand) noexcept
        : ast_unary_expression(eExprType::LogicalNot, operand) {}
};

struct ast_prefix_increment_expression : ast_unary_expression {
    ast_prefix_increment_expression(ast_expression *operand) noexcept
        : ast_unary_expression(eExprType::PrefixIncrement, operand) {}
};

struct ast_prefix_decrement_expression : ast_unary_expression {
    ast_prefix_decrement_expression(ast_expression *operand) noexcept
        : ast_unary_expression(eExprType::PrefixDecrement, operand) {}
};

struct ast_sequence_expression : ast_binary_expression {
    ast_sequence_expression() noexcept : ast_binary_expression(eExprType::Sequence) {}
};

struct ast_assignment_expression : ast_binary_expression {
    eOperator oper;

    ast_assignment_expression(eOperator _oper) noexcept : ast_binary_expression(eExprType::Assign), oper(_oper) {}
};

struct ast_operation_expression : ast_binary_expression {
    eOperator oper;

    ast_operation_expression(eOperator _oper) noexcept : ast_binary_expression(eExprType::Operation), oper(_oper) {}
};

struct ast_ternary_expression : ast_expression {
    ast_expression *condition = nullptr, *on_true = nullptr, *on_false = nullptr;

    ast_ternary_expression() noexcept : ast_expression(eExprType::Ternary) {}
};
} // namespace glslx