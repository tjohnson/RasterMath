/*
 * The information in this file is
 * Copyright(c) 2009 Todd A. Johnson
 * and is subject to the terms and conditions of the
 * GNU Lesser General Public License Version 2.1
 * The license text is available from   
 * http://www.gnu.org/licenses/lgpl.html
 */

#include "Progress.h"
#include "RasterMathException.h"
#include "RasterMathProgress.h"

RasterMathProgress::RasterMathProgress(Progress* pProgress, bool& aborted, int64_t totalWork) :
   mpProgress(pProgress),
   mTotalWork(totalWork),
   mPreviousWork(0),
   mPreviousReportedWork(0),
   mAborted(aborted)
{
   RM_VERIFY(totalWork > 0);
}

bool RasterMathProgress::setWorkCompleted(int64_t workCompleted)
{
   if (mpProgress)
   {
      if (workCompleted >= 2000000+mPreviousReportedWork || workCompleted >= mTotalWork)
      {
         int percent = static_cast<int>(100*workCompleted/mTotalWork);
         mpProgress->updateProgress("Raster Math is computing...", percent, NORMAL);
         mPreviousReportedWork = workCompleted;
      }
      mPreviousWork = workCompleted;
      if (mAborted)
      {
         mpProgress->updateProgress("Raster Math is aborted", 100, ABORT);
      }
   }

   return mAborted;
}
