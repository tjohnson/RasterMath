/*
 * The information in this file is
 * Copyright(c) 2009 Todd A. Johnson
 * and is subject to the terms and conditions of the
 * GNU Lesser General Public License Version 2.1
 * The license text is available from   
 * http://www.gnu.org/licenses/lgpl.html
 */

#ifndef RASTERMATHPLUGIN_H
#define RASTERMATHPLUGIN_H

#include "AlgorithmShell.h"

#include <string>

class Progress;
class RasterElement;
class RasterMathRunner;

using namespace std;

class RasterMathPlugIn : public AlgorithmShell
{
public:
   RasterMathPlugIn();
   ~RasterMathPlugIn();

   bool execute(PlugInArgList* pInParam, PlugInArgList* pOutParam);
   bool getInputSpecification(PlugInArgList*& pArgList);
   bool getOutputSpecification(PlugInArgList*& pArgList);

private:
   bool batchExecute(PlugInArgList* pInParam, PlugInArgList* pOutParam, RasterMathRunner& runner);

   Progress* mpProgress;
   RasterElement* mpRaster;
   std::string mResultsName;
   std::string mFormula;
   int mDisplayLayer;
   EncodingType mResultEncoding;
};

#endif
