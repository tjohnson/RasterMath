/*
 * The information in this file is
 * Copyright(c) 2009 Todd A. Johnson
 * and is subject to the terms and conditions of the
 * GNU Lesser General Public License Version 2.1
 * The license text is available from   
 * http://www.gnu.org/licenses/lgpl.html
 */

#ifndef RASTERMATHPROGRESS_H
#define RASTERMATHPROGRESS_H

#include "AppConfig.h"

class Progress;

class RasterMathProgress
{
public:
   RasterMathProgress(Progress* pProgress, bool& aborted, int64_t totalWork);
   bool setWorkCompleted(int64_t workCompleted);
   bool addWorkCompleted(int64_t workCompleted)
   {
      return setWorkCompleted(workCompleted + mPreviousWork);
   }

private:
   Progress* mpProgress;
   int64_t mTotalWork;
   int64_t mPreviousWork;
   int64_t mPreviousReportedWork;
   bool& mAborted;
};

#endif
