/*
 * The information in this file is
 * Copyright(c) 2007 Ball Aerospace & Technologies Corporation
 * and is subject to the terms and conditions of the
 * GNU Lesser General Public License Version 2.1
 * The license text is available from   
 * http://www.gnu.org/licenses/lgpl.html
 */

#ifndef PROCESS_STAT_STEP_FUNC_H
#define PROCESS_STAT_STEP_FUNC_H

#include "ProcessStack.h"
#include "ProcessStep.h"

#include <deque>

class RasterMathProgress;

class ProcessStepStatFunc : public ProcessStepFunction
{
   friend class ProcessStack;
public:
   ProcessStepStatFunc(const std::string& description, StepType type, const std::vector<boost::shared_ptr<ProcessStep> >& args, int argCount);
   void setSubStack(const std::vector<boost::shared_ptr<ProcessStep> >& subStack);
   void execute(RasterMathProgress& progress);
   void initializeAccumulators();
   int64_t oneTimeWork() const;
   bool operator==(const ProcessStep& rhs) const;

protected:
   std::deque<double> mValues;
   ProcessStack mSubStack;
   double mAccumulator1;
   double mAccumulator2;
   double mAccumulator3;
   int mSubStackBands;
   int mSubStackRows;
   int mSubStackColumns;
};

#endif
