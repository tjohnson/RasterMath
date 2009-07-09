/*
 * The information in this file is
 * Copyright(c) 2007 Ball Aerospace & Technologies Corporation
 * and is subject to the terms and conditions of the
 * GNU Lesser General Public License Version 2.1
 * The license text is available from   
 * http://www.gnu.org/licenses/lgpl.html
 */

#ifndef RASTERMATHPARSER_H
#define RASTERMATHPARSER_H

#include <string>
#include <vector>

class ProcessStack;

class RasterMathParser
{
public:
   RasterMathParser(const std::string& formula);

   ProcessStack& getProcessStack() const;
   std::string getFormula() const;

private:
   std::string mFormula;

   void parseFormula();
};

#endif
