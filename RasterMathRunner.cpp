/*
 * The information in this file is
 * Copyright(c) 2009 Todd A. Johnson
 * and is subject to the terms and conditions of the
 * GNU Lesser General Public License Version 2.1
 * The license text is available from   
 * http://www.gnu.org/licenses/lgpl.html
 */

#include "DesktopServices.h"
#include "DockWindow.h"
#include "MessageLogResource.h"
#include "ParseStackBuilder.h"
#include "PlotSetGroup.h"
#include "ProcessStack.h"
#include "ProcessStep.h"
#include "RasterElement.h"
#include "RasterMathException.h"
#include "RasterMathParser.h"
#include "RasterMathProgress.h"
#include "RasterMathRunner.h"
#include "Signature.h"
#include "SpatialDataView.h"
#include "SpatialDataWindow.h"

#include <QtGui/QMessageBox>
#include <QtCore/QTime>

using namespace std;

RasterMathRunner::RasterMathRunner(Progress* pProgress, bool& aborted) :
   mDisplayType(DISPLAY_NONE),
   mBaseResultName("Raster Math Results"),
   mResultEncoding(FLT4BYTES),
   mpProgress(pProgress),
   mDefaultValue(0.0),
   mFailOnError(false),
   mRadians(true),
   mpRasterResult(NULL),
   mpSignatureResult(NULL),
   mScalarResult(0.0),
   mAborted(aborted)
{
}

void RasterMathRunner::execute(const std::string &formula)
{
   MessageResource mr(QString("Formula: %1").arg(QString::fromStdString(formula)).toStdString(), 
      "RasterMath", "{5BA07186-5B91-4559-8F64-0317F0DE92D1}");

   ParseStackBuilder::clear();

   RasterMathParser parser(formula);
   ProcessStack& stack = parser.getProcessStack();
   stack.setFailureMode(mFailOnError, mDefaultValue);
   stack.setDegrees(!mRadians);

   const std::vector<boost::shared_ptr<ProcessStep> >& steps = stack.getSteps();
   if (steps.empty())
   {
      throw RasterMathException("Formula is empty");
   }
   else
   {
      RM_VERIFY(!steps.empty()&&steps.back());
      if (steps.back()->isScalar())
      {
         double result = executeScalar(stack);
         QString message = QString::fromStdString(formula) + " = ";
         message += QString::number(result);
         QMessageBox::information(Service<DesktopServices>()->getMainWidget(), "Raster Math", message);
      }
      else if (steps.back()->isSignature())
      {
         executeFull(stack);
         displaySignature(stack.releaseSignature());
      }
      else
      {
         executeFull(stack);
         displayRaster(stack.releaseRaster());
      }
   }
}

double RasterMathRunner::executeScalar(ProcessStack& stack)
{
   const std::vector<boost::shared_ptr<ProcessStep> >& steps = stack.getSteps();
   vector<double> workingStack;
   workingStack.reserve(steps.size());
   stack.addResultStep("", mResultEncoding, mResultLocation);
   stack.compute(workingStack, RasterMathProgress(mpProgress, mAborted, 1));
   RM_VERIFY(!steps.empty());
   return RM_NULLCHK(steps.back().get())->value();
}

void RasterMathRunner::executeFull(ProcessStack& stack)
{
   QTime startTime = QTime::currentTime();

   stack.addResultStep(mBaseResultName, mResultEncoding, mResultLocation);
   int64_t totalWork = stack.totalWork();
   mAborted = false;
   RasterMathProgress progress(mpProgress, mAborted, totalWork);
   stack.execute(progress);

   QTime stopTime = QTime::currentTime();
   QString message = QString("Elapsed time: %1 seconds").arg(startTime.msecsTo(stopTime)/1000.0);
   MessageResource mr1(message.toStdString(), "RasterMath", "{FE28FF96-2352-4348-BF22-89D24C3295D1}");
   message = QString("Total Work: %1 operations").arg(static_cast<double>(totalWork));
   MessageResource mr2(message.toStdString(), "RasterMath", "{6029D5B0-2C44-4e9b-93E1-BAFCEE92C40B}");
}

void RasterMathRunner::setDisplayType(DisplayType type)
{
   mDisplayType = type;
}

void RasterMathRunner::displaySignature(Signature* pSig)
{
   if (mDisplayType != DISPLAY_NONE)
   {
      PlotSetGroup* pPlotSetGroup = NULL;
      DockWindow* pWindow =
         dynamic_cast<DockWindow*>(Service<DesktopServices>()->createWindow("Raster Math Result", DOCK_WINDOW));
      if (pWindow != NULL)
      {
         pPlotSetGroup = Service<DesktopServices>()->createPlotSetGroup();
         if (pPlotSetGroup != NULL)
         {
            pWindow->setWidget(pPlotSetGroup->getWidget());
         }
      }
      else
      {
         pWindow = dynamic_cast<DockWindow*>(Service<DesktopServices>()->getWindow("Raster Math Result", DOCK_WINDOW));
         if (pWindow != NULL)
         {
            pPlotSetGroup = dynamic_cast<PlotSetGroup*>(pWindow->getWidget());
         }
      }

      if (pPlotSetGroup != NULL)
      {
         pPlotSetGroup->plotData(*pSig, "Raster Math Indices", "Raster Math Values", pSig->getName());
      }
   }
}

void RasterMathRunner::displayRaster(RasterElement* pElement)
{
   Service<DesktopServices> pDesktop;
   LayerType layerType = RASTER;
   switch(mDisplayType)
   {
      case DISPLAY_NONE:
         break;
      case DISPLAY_THRESHOLD_OVERLAY:
         layerType = THRESHOLD;
      case DISPLAY_RASTER_OVERLAY:
      {
         SpatialDataView *pView = dynamic_cast<SpatialDataView*>(pDesktop->getCurrentWorkspaceWindowView());
         if (pView != NULL)
         {
            pView->createLayer(layerType, pElement);
         }
         break;
      }
      case DISPLAY_RASTER_WINDOW:
      {
         WorkspaceWindow* pOldCurrentWindow = pDesktop->getCurrentWorkspaceWindow();
         SpatialDataWindow *pWindow = dynamic_cast<SpatialDataWindow*>(pDesktop->createWindow(pElement->getName(), SPATIAL_DATA_WINDOW));
         if (pWindow != NULL)
         {
            SpatialDataView *pView = RM_NULLCHK(dynamic_cast<SpatialDataView*>(pWindow->getView()));
            pView->setPrimaryRasterElement(pElement);
            pView->createLayer(layerType, pElement);
            if (pOldCurrentWindow != NULL)
            {
               pDesktop->setCurrentWorkspaceWindow(pOldCurrentWindow);
            }
         }
         break;
      }
      default:
         break;
   }
}

void RasterMathRunner::setBaseResultName(const std::string& baseName)
{
   mBaseResultName = baseName;
}

void RasterMathRunner::setRadians(bool radians)
{
   mRadians = radians;
}
