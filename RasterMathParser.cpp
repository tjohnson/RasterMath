/*
 * The information in this file is
 * Copyright(c) 2007 Ball Aerospace & Technologies Corporation
 * and is subject to the terms and conditions of the
 * GNU Lesser General Public License Version 2.1
 * The license text is available from   
 * http://www.gnu.org/licenses/lgpl.html
 */

#include "ParseStackBuilder.h"
#include "RasterMathException.h"
#include "RasterMathGrammar.h"
#include "RasterMathParser.h"

using namespace std;
using namespace boost;
using namespace boost::spirit;

RasterMathParser::RasterMathParser(const string& formula) :
   mFormula(formula)
{
   parseFormula();
}

string RasterMathParser::getFormula() const
{
   return mFormula;
}

ProcessStack& RasterMathParser::getProcessStack() const
{
   return ParseStackBuilder::getStack();
}

void RasterMathParser::parseFormula()
{
   RmGrammar grammar;
   parse_info<> info = parse(mFormula.c_str(), grammar, space_p);
   if (!info.full)
   {
      throw RasterMathException("Unable to parse formula:\n" + mFormula);
   }
}