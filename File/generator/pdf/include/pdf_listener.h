#pragma once

#include "antlr4-runtime.h"
#include "generator.h"
#include "pdf_parser.h"


class  pdf_listener : public antlr4::tree::ParseTreeListener {
public:

  IR *ir;

  virtual void exitPdf(pdf_parser::PdfContext *ctx);

  virtual void exitPdf_body(pdf_parser::Pdf_bodyContext *ctx);

  virtual void exitPdf_header(pdf_parser::Pdf_headerContext *ctx);

  virtual void exitPdf_obj(pdf_parser::Pdf_objContext *ctx);

  virtual void exitPdf_obj_name(pdf_parser::Pdf_obj_nameContext *ctx);

  virtual void exitPdf_obj_string(pdf_parser::Pdf_obj_stringContext *ctx);

  virtual void exitPdf_obj_array(pdf_parser::Pdf_obj_arrayContext *ctx);

  virtual void exitPdf_obj_dictionary(pdf_parser::Pdf_obj_dictionaryContext *ctx);

  virtual void exitPdf_obj_reference(pdf_parser::Pdf_obj_referenceContext *ctx);

  virtual void exitPdf_obj_stream(pdf_parser::Pdf_obj_streamContext *ctx);

  virtual void exitPdf_xref_table(pdf_parser::Pdf_xref_tableContext *ctx);

  virtual void exitPdf_trailer(pdf_parser::Pdf_trailerContext *ctx);

};