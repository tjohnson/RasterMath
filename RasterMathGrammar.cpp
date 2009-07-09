/*
 * The information in this file is
 * Copyright(c) 2007 Ball Aerospace & Technologies Corporation
 * and is subject to the terms and conditions of the
 * GNU Lesser General Public License Version 2.1
 * The license text is available from   
 * http://www.gnu.org/licenses/lgpl.html
 */

#include "ParseStackBuilder.h"
#include "RasterMathGrammar.h"

using namespace boost::spirit;

template <>
RmGrammar
::definition<class boost::spirit::scanner<char const *,struct boost::spirit::scanner_policies<struct boost::spirit::skipper_iteration_policy<struct boost::spirit::iteration_policy>,struct boost::spirit::match_policy,struct boost::spirit::action_policy> > >
::definition(RmGrammar const& self)
{
   refName = (str_p("r1") | "r2" | "r3" | "r4" | "r5");
   ref1 = refName[&ParseStackBuilder::fullRaster];
   ref2 = (refName >> '[' >> number >> ']')[&ParseStackBuilder::rasterIndex];
   ref3 = (refName >> '[' >> number >> ':' >> number >> ']')[&ParseStackBuilder::rasterFullSlice];
   ref4 = (refName >> '[' >> number >> ':' >> ']')[&ParseStackBuilder::rasterNtoEndSlice];
   ref5 = (refName >> '[' >> ':' >> number >> ']')[&ParseStackBuilder::raster0toNSlice];
   aoiName = (str_p("a1") | "a2" | "a3" | "a4" | "a5")[&ParseStackBuilder::aoi];
   fullref = ref3 | ref5 | ref4 | ref2 | ref1 | aoiName; // leave in this order: it minimizes number-action duplication

   func1 = ((str_p("abs") | "sqrt" | "exp" | "log10" | "log2" | "log" | 
      "acos" | "cos" | "asin" | "sin" | "atan" | "tan" | "cosh" | "sinh" | "tanh") >> group)[&ParseStackBuilder::func1];
   statfunc1 = ((str_p("min") | "max" | "mean" | "avg" | "geomean" | "harmean" | "sum" | "stdev") >> group)[&ParseStackBuilder::statfunc1];
   func2 = ((str_p("atan2") | "logn") >> '(' >> fullexpr >> ',' >> fullexpr >> ')')[&ParseStackBuilder::func2];
   func3 = ((str_p("clamp")) >> '(' >> fullexpr >> ',' >> fullexpr >> ',' >> fullexpr >> ')')[&ParseStackBuilder::func3];
   function = (func1 | func2 | func3 | statfunc1);

   group = '(' >> fullexpr >> ')';

   number = lexeme_d[(real_p)[&ParseStackBuilder::number]];
   constant = ((str_p("pi"))[&ParseStackBuilder::pi] | 
      (str_p("e"))[&ParseStackBuilder::e]);

   expr1 = fullref | group | function | constant | number;
   expr2 = expr1 >> *(('^' >> expr1)[&ParseStackBuilder::exponentiate]);
   expr3 = expr2 >> *(('*' >> expr2)[&ParseStackBuilder::multiply] | 
      ('/' >> expr2)[&ParseStackBuilder::divide] | 
      ('%' >> expr2)[&ParseStackBuilder::modulo]);
   expr4 = expr3 | 
      ('-' >> expr3)[&ParseStackBuilder::negate] |
      ('+' >> expr3);
   expr5 = expr4 >> *(('+' >> expr4)[&ParseStackBuilder::add] | 
      ('-' >> expr4)[&ParseStackBuilder::subtract]);
   expr6 = expr5 >> !(('=' >> expr5)[&ParseStackBuilder::equals] | 
      ("!=" >> expr5)[&ParseStackBuilder::notEquals] | 
      ('<' >> expr5)[&ParseStackBuilder::lessThan] | 
      ('>' >> expr5)[&ParseStackBuilder::greaterThan] | 
      ("<=" >> expr5)[&ParseStackBuilder::lessOrEqual] | 
      (">=" >> expr5)[&ParseStackBuilder::greaterOrEqual]);
   expr7 = expr6 |
      (('!' | str_p("not")) >> expr6)[&ParseStackBuilder::not];
   expr8 = expr7 >> *((('|' | str_p("or")) >> expr7)[&ParseStackBuilder::or] |
      (('&' | str_p("and")) >> expr7)[&ParseStackBuilder::and]);

   fullexpr = expr8;
}

/*
ceil
floor
fact
rows
columns
bands
*/