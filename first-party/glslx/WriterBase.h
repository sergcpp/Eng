#pragma once

#include <iosfwd>
#include <string>

#include "Bitmask.h"
#include "parser/AST.h"

namespace glslx {
enum class eOutputFlags { WriteTabs, VarType, VarName, VarArrSize, Coma, Semicolon, NewLine };
const Bitmask<eOutputFlags> DefaultOutputFlags = Bitmask{eOutputFlags::WriteTabs} | eOutputFlags::VarType |
                                                 eOutputFlags::VarName | eOutputFlags::VarArrSize |
                                                 eOutputFlags::Semicolon | eOutputFlags::NewLine;

struct writer_config_t {
    std::string tab = "    ";
    bool write_hidden = false;
    bool drop_half_float_literals = false;
};

class WriterBase {
  protected:
    writer_config_t config_;
    int nest_level_ = 0;

    void Write_Tabs(std::ostream &out_stream);
    void Write_Constant(const ast_short_constant *expression, std::ostream &out_stream);
    void Write_Constant(const ast_ushort_constant *expression, std::ostream &out_stream);
    void Write_Constant(const ast_int_constant *expression, std::ostream &out_stream);
    void Write_Constant(const ast_uint_constant *expression, std::ostream &out_stream);
    void Write_Constant(const ast_long_constant *expression, std::ostream &out_stream);
    void Write_Constant(const ast_ulong_constant *expression, std::ostream &out_stream);
    void Write_Constant(const ast_half_constant *expression, std::ostream &out_stream);
    void Write_Constant(const ast_float_constant *expression, std::ostream &out_stream);
    void Write_Constant(const ast_double_constant *expression, std::ostream &out_stream);
    void Write_Constant(const ast_bool_constant *expression, std::ostream &out_stream);

  public:
    explicit WriterBase(writer_config_t config) : config_(std::move(config)) {}
};
} // namespace glslx