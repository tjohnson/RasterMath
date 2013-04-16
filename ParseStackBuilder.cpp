/*
 * The information in this file is
 * Copyright(c) 2009 Todd A. Johnson
 * and is subject to the terms and conditions of the
 * GNU Lesser General Public License Version 2.1
 * The license text is available from   
 * http://www.gnu.org/licenses/lgpl.html
 */

#include "AppVerify.h"
#include "ParseStackBuilder.h"
#include "ProcessStep.h"
#include "ProcessStepStatFunc.h"
#include "RasterMathException.h"

#include <math.h>

#include <sstream>

using namespace std;

namespace
{
   ProcessStack sStack;
   
   string getFirstWord(char const* pStr, char const* pEnd)
   {
      while (pStr < pEnd && *pStr == ' ') ++pStr;
      char const* pWordEnd = pStr+1;
      while (pWordEnd < pEnd && *pWordEnd != ' ' && isalnum(*pWordEnd)) ++pWordEnd;
      return string(pStr, pWordEnd);
   }

   int getBandIndex(ProcessStack& stack)
   {
      boost::shared_ptr<ProcessStep> pIndexStep = stack.getSteps().back();
      RM_NULLCHK(pIndexStep);
      if (pIndexStep->type() != ProcessStep::NUMBER)
      {
         throw RasterMathException("Invalid band index");
      }
      int index = static_cast<int>(static_cast<ProcessStepNumber*>(pIndexStep.get())->value());
      stack.pop_back();
      return index;
   }

   vector<boost::shared_ptr<ProcessStep> > getSubStack(const vector<boost::shared_ptr<ProcessStep> >& steps, int argCount)
   {
      vector<boost::shared_ptr<ProcessStep> >::const_iterator ppStart, ppStop;
      ppStart = ppStop = steps.end();
      int currentCount = argCount;
      while (currentCount != 0)
      {
         if (ppStart == steps.begin())
         {
            throw RasterMathException("Parse failure");
         }
         --ppStart;
         --currentCount;
         currentCount += RM_NULLCHK(*ppStart)->argCount();
      }
      return vector<boost::shared_ptr<ProcessStep> >(ppStart, ppStop);
   }

   ProcessStep::StepType typeFromName(const string& fname, const char* pNames[], const ProcessStep::StepType* pTypes, int typeCount)
   {
      vector<string> fnames(pNames, &pNames[typeCount]);
      vector<ProcessStep::StepType> stepTypes(pTypes, &pTypes[typeCount]);
      vector<ProcessStep::StepType>::const_iterator typeIter=stepTypes.begin();
      for (vector<string>::const_iterator nameIter=fnames.begin(); 
         nameIter!=fnames.end() && typeIter!=stepTypes.end(); ++nameIter, ++typeIter)
      {
         if (*nameIter == fname)
         {
            return *typeIter;
         }
      }
      return ProcessStep::NUMBER;
   }

   template<class T>
   boost::shared_ptr<ProcessStep> createFunc(char const* pStr, char const* pEnd, const char* pNames[], const ProcessStep::StepType* pTypes, int typeCount, const vector<boost::shared_ptr<ProcessStep> > &steps, int argCount)
   {
      string name = getFirstWord(pStr, pEnd);
      return boost::shared_ptr<ProcessStep>(new T(name, typeFromName(name, pNames, pTypes, typeCount), steps, argCount));
   }
}

void ParseStackBuilder::clear()
{
   sStack.clear();
}

ProcessStack& ParseStackBuilder::getStack()
{
   return sStack;
}

void ParseStackBuilder::integer(int d)
{
   stringstream s;
   s << d;
   sStack.add(boost::shared_ptr<ProcessStep>(new ProcessStepNumber(s.str(), ProcessStep::NUMBER, d)));
}

void ParseStackBuilder::number(double d)
{
   stringstream s;
   s << d;
   sStack.add(boost::shared_ptr<ProcessStep>(new ProcessStepNumber(s.str(), ProcessStep::NUMBER, d)));
}

void ParseStackBuilder::pi(char const* pStr, char const* pEnd)
{
   sStack.add(boost::shared_ptr<ProcessStep>(new ProcessStepNumber("pi", ProcessStep::NUMBER, 2.0*atan2(1.0,0.0))));
}

void ParseStackBuilder::e(char const* pStr, char const* pEnd)
{
   sStack.add(boost::shared_ptr<ProcessStep>(new ProcessStepNumber("e", ProcessStep::NUMBER, exp(1.0))));
}

void ParseStackBuilder::negate(char const* pStr, char const* pEnd)
{
   sStack.add(boost::shared_ptr<ProcessStep>(new ProcessStepFunction("negate", ProcessStep::NEGATE, sStack.getSteps(), 1)));
}

void ParseStackBuilder::add(char const* pStr, char const* pEnd)
{
   sStack.add(boost::shared_ptr<ProcessStep>(new ProcessStepFunction("add", ProcessStep::ADD, sStack.getSteps(), 2)));
}

void ParseStackBuilder::subtract(char const* pStr, char const* pEnd)
{
   sStack.add(boost::shared_ptr<ProcessStep>(new ProcessStepFunction("subtract", ProcessStep::SUBTRACT, sStack.getSteps(), 2)));
}

void ParseStackBuilder::multiply(char const* pStr, char const* pEnd)
{
   sStack.add(boost::shared_ptr<ProcessStep>(new ProcessStepFunction("multiply", ProcessStep::MULTIPLY, sStack.getSteps(), 2)));
}

void ParseStackBuilder::divide(char const* pStr, char const* pEnd)
{
   sStack.add(boost::shared_ptr<ProcessStep>(new ProcessStepFunction("divide", ProcessStep::DIVIDE, sStack.getSteps(), 2)));
}

void ParseStackBuilder::modulo(char const* pStr, char const* pEnd)
{
   sStack.add(boost::shared_ptr<ProcessStep>(new ProcessStepFunction("modulo", ProcessStep::MODULO, sStack.getSteps(), 2)));
}

void ParseStackBuilder::exponentiate(char const* pStr, char const* pEnd)
{
   sStack.add(boost::shared_ptr<ProcessStep>(new ProcessStepFunction("exponentiate", ProcessStep::EXPONENTIATE, sStack.getSteps(), 2)));
}

void ParseStackBuilder::equals(char const* pStr, char const* pEnd)
{
   sStack.add(boost::shared_ptr<ProcessStep>(new ProcessStepFunction("equals", ProcessStep::EQUALS, sStack.getSteps(), 2)));
}

void ParseStackBuilder::notEquals(char const* pStr, char const* pEnd)
{
   sStack.add(boost::shared_ptr<ProcessStep>(new ProcessStepFunction("not equals", ProcessStep::NOT_EQUALS, sStack.getSteps(), 2)));
}

void ParseStackBuilder::lessThan(char const* pStr, char const* pEnd)
{
   sStack.add(boost::shared_ptr<ProcessStep>(new ProcessStepFunction("less than", ProcessStep::LESS_THAN, sStack.getSteps(), 2)));
}

void ParseStackBuilder::greaterThan(char const* pStr, char const* pEnd)
{
   sStack.add(boost::shared_ptr<ProcessStep>(new ProcessStepFunction("greater than", ProcessStep::GREATER_THAN, sStack.getSteps(), 2)));
}

void ParseStackBuilder::lessOrEqual(char const* pStr, char const* pEnd)
{
   sStack.add(boost::shared_ptr<ProcessStep>(new ProcessStepFunction("less or equal", ProcessStep::LESS_OR_EQUAL, sStack.getSteps(), 2)));
}

void ParseStackBuilder::greaterOrEqual(char const* pStr, char const* pEnd)
{
   sStack.add(boost::shared_ptr<ProcessStep>(new ProcessStepFunction("greater or equal", ProcessStep::GREATER_OR_EQUAL, sStack.getSteps(), 2)));
}

void ParseStackBuilder::not(char const* pStr, char const* pEnd)
{
   sStack.add(boost::shared_ptr<ProcessStep>(new ProcessStepFunction("not", ProcessStep::NOT, sStack.getSteps(), 1)));
}

void ParseStackBuilder::and(char const* pStr, char const* pEnd)
{
   sStack.add(boost::shared_ptr<ProcessStep>(new ProcessStepFunction("and", ProcessStep::AND, sStack.getSteps(), 2)));
}

void ParseStackBuilder::or(char const* pStr, char const* pEnd)
{
   sStack.add(boost::shared_ptr<ProcessStep>(new ProcessStepFunction("or", ProcessStep::OR, sStack.getSteps(), 2)));
}

void ParseStackBuilder::func1(char const* pStr, char const* pEnd)
{
   static const char* pNames[] = {"abs", "sqrt", "acos", "cos", "asin", "sin", "atan", "tan", "cosh", "sinh", "tanh", "exp", "log10", "log2", "log"};
   static const ProcessStep::StepType pTypes[] = {ProcessStep::ABS, ProcessStep::SQRT, ProcessStep::ACOS, ProcessStep::COS, ProcessStep::ASIN, ProcessStep::SIN, ProcessStep::ATAN, ProcessStep::TAN, ProcessStep::COSH, ProcessStep::SINH, ProcessStep::TANH, ProcessStep::EXP, ProcessStep::LOG10, ProcessStep::LOG2, ProcessStep::LOG};
   VERIFYNRV(sizeof(pNames)/sizeof(pNames[0]) == sizeof(pTypes)/sizeof(pTypes[0]));
   sStack.add(createFunc<ProcessStepFunction>(pStr, pEnd, pNames, pTypes, sizeof(pNames)/sizeof(pNames[0]), sStack.getSteps(), 1));
}

void ParseStackBuilder::func2(char const* pStr, char const* pEnd)
{
   static const char* pNames[] = {"atan2", "logn"};
   static const ProcessStep::StepType pTypes[] = {ProcessStep::ATAN2, ProcessStep::LOGN};
   VERIFYNRV(sizeof(pNames)/sizeof(pNames[0]) == sizeof(pTypes)/sizeof(pTypes[0]));
   sStack.add(createFunc<ProcessStepFunction>(pStr, pEnd, pNames, pTypes, sizeof(pNames)/sizeof(pNames[0]), sStack.getSteps(), 2));
}

void ParseStackBuilder::func3(char const* pStr, char const* pEnd)
{
   static const char* pNames[] = {"clamp"};
   static const ProcessStep::StepType pTypes[] = {ProcessStep::CLAMP};
   VERIFYNRV(sizeof(pNames)/sizeof(pNames[0]) == sizeof(pTypes)/sizeof(pTypes[0]));
   sStack.add(createFunc<ProcessStepFunction>(pStr, pEnd, pNames, pTypes, sizeof(pNames)/sizeof(pNames[0]), sStack.getSteps(), 3));
}

void ParseStackBuilder::statfunc1(char const* pStr, char const* pEnd)
{
   const vector<boost::shared_ptr<ProcessStep> > &steps = sStack.getSteps();
   vector<boost::shared_ptr<ProcessStep> > subStack = getSubStack(steps, 1);

   static const char* pNames[] = {"min", "max", "mean", "avg", "geomean", "harmean", "sum", "stdev"};
   static const ProcessStep::StepType pTypes[] = {ProcessStep::BAND_MIN, ProcessStep::BAND_MAX, ProcessStep::BAND_MEAN, ProcessStep::BAND_MEAN, ProcessStep::BAND_GEOMEAN, ProcessStep::BAND_HARMEAN, ProcessStep::BAND_SUM, ProcessStep::BAND_STDDEV};
   VERIFYNRV(sizeof(pNames)/sizeof(pNames[0]) == sizeof(pTypes)/sizeof(pTypes[0]));
   boost::shared_ptr<ProcessStep> pStep = createFunc<ProcessStepStatFunc>(pStr, pEnd, pNames, pTypes, sizeof(pNames)/sizeof(pNames[0]), sStack.getSteps(), 1);
   RM_NULLCHK(pStep);

   int subStackSize = subStack.size();
   subStack.push_back(pStep);

   ProcessStepStatFunc* pStatFunc = static_cast<ProcessStepStatFunc*>(pStep.get());

   pStatFunc->setSubStack(subStack);

   for (int i=0; i<subStackSize; ++i)
   {
      sStack.pop_back();
   }

   sStack.add(pStep);
}

void ParseStackBuilder::fullRaster(char const* pStr, char const* pEnd)
{
   sStack.add(boost::shared_ptr<ProcessStep>(new ProcessStepRaster(getFirstWord(pStr, pEnd), ProcessStep::VALUE_RASTER, 0, -1)));
}

void ParseStackBuilder::rasterIndex(char const* pStr, char const* pEnd)
{
   // grammar parses index thrice, pitch the two extras
   sStack.pop_back();
   sStack.pop_back();

   int index = getBandIndex(sStack);
   RM_VERIFY(index>=1);
   sStack.add(boost::shared_ptr<ProcessStep>(new ProcessStepRaster(getFirstWord(pStr, pEnd), ProcessStep::VALUE_RASTER, index-1, index-1)));
}

void ParseStackBuilder::rasterFullSlice(char const* pStr, char const* pEnd)
{
   int maxIndex = getBandIndex(sStack);
   int minIndex = getBandIndex(sStack);
   RM_VERIFY(maxIndex>=1);
   RM_VERIFY(minIndex>=1);
   sStack.add(boost::shared_ptr<ProcessStep>(new ProcessStepRaster(getFirstWord(pStr, pEnd), ProcessStep::VALUE_RASTER, minIndex-1, maxIndex-1)));
}

void ParseStackBuilder::rasterNtoEndSlice(char const* pStr, char const* pEnd)
{
   // grammar parses index twice, pitch the extra
   sStack.pop_back();

   int index = getBandIndex(sStack);
   RM_VERIFY(index>=1);
   sStack.add(boost::shared_ptr<ProcessStep>(new ProcessStepRaster(getFirstWord(pStr, pEnd), ProcessStep::VALUE_RASTER, index-1, -1)));
}

void ParseStackBuilder::raster0toNSlice(char const* pStr, char const* pEnd)
{
   int index = getBandIndex(sStack);
   RM_VERIFY(index>=1);
   sStack.add(boost::shared_ptr<ProcessStep>(new ProcessStepRaster(getFirstWord(pStr, pEnd), ProcessStep::VALUE_RASTER, 0, index-1)));
}

void ParseStackBuilder::aoi(char const* pStr, char const* pEnd)
{
   sStack.add(boost::shared_ptr<ProcessStep>(new ProcessStepAoi(getFirstWord(pStr, pEnd))));
}
