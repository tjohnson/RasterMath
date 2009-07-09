/*
 * The information in this file is
 * Copyright(c) 2009 Todd A. Johnson
 * and is subject to the terms and conditions of the
 * GNU Lesser General Public License Version 2.1
 * The license text is available from   
 * http://www.gnu.org/licenses/lgpl.html
 */

#include "AppVersion.h"
#include "AppVerify.h"
#include "DesktopServices.h"
#include "MessageLogResource.h"
#include "ParseStackBuilder.h"
#include "PlugInArg.h"
#include "PlugInArgList.h"
#include "PlugInManagerServices.h"
#include "PlugInRegistration.h"
#include "ProcessStep.h"
#include "Progress.h"
#include "RasterCorrelator.h"
#include "RasterElement.h"
#include "RasterMathDlgImp.h"
#include "RasterMathException.h"
#include "RasterMathParser.h"
#include "RasterMathPlugIn.h"
#include "RasterMathRunner.h"

#include <QtGui/QMessageBox>
#include <sstream>

REGISTER_PLUGIN_BASIC(RasterMath, RasterMathPlugIn);

namespace
{
   const string RESULTS_NAME = "Results Name";
   const string FORMULA = "Formula";
   const string DISPLAY_LAYER = "Display Layer";
   const string RESULT_ENCODING = "Result Encoding";
   const string FAIL_ON_ERROR = "Fail on Error";
   const string DEFAULT_VALUE = "Default Value";
   const string RADIANS = "Radians";
   const string LOCATION = "Location";
   const string RASTER_ARG = "Raster ";
   const string RASTER2 = RASTER_ARG+"2";
   const string RASTER3 = RASTER_ARG+"3";
   const string RASTER4 = RASTER_ARG+"4";
   const string RASTER5 = RASTER_ARG+"5";
   const string AOI_ARG = "Aoi ";
   const string AOI1 = AOI_ARG+"1";
   const string AOI2 = AOI_ARG+"2";
   const string AOI3 = AOI_ARG+"3";
   const string AOI4 = AOI_ARG+"4";
   const string AOI5 = AOI_ARG+"5";
   const string SCALAR_RESULT = "Scalar Result";
   const string SIGNATURE_RESULT = "Signature Result";
   const string RASTER_RESULT = "Raster Result";
   const int MAX_ARG = 5;

   template<class T>
   string toString(const T& t)
   {
      stringstream s;
      s << t;
      return s.str();
   }

   template<class T> 
   void addCorrelation(map<int,T*>& correlations, int index, PlugInArgList& argList, const string& argName)
   {
      T* pElement = argList.getPlugInArgValue<T>(argName);
      if (pElement != NULL)
      {
         correlations[index] = pElement;
      }
   }
}

RasterMathPlugIn::RasterMathPlugIn() :
   mpProgress(NULL),
   mpRaster(NULL),
   mResultsName("Raster Math Result"),
   mDisplayLayer(0),
   mResultEncoding(FLT4BYTES)
{
   setName("RasterMath");
   setCreator("Ball Aerospace & Technologies Corp.");
   setCopyright(APP_COPYRIGHT);
   setVersion(APP_VERSION_NUMBER);
   setMenuLocation("[General Algorithms]\\Raster Math");
   setDescriptorId("{C7AC6265-BCB0-49f6-861E-FE3BC3A0AC21}");
   setAbortSupported(true);
   allowMultipleInstances(true);
   setProductionStatus(APP_IS_PRODUCTION_RELEASE);
}

RasterMathPlugIn::~RasterMathPlugIn()
{
}

bool RasterMathPlugIn::execute(PlugInArgList* pInParam, PlugInArgList* pOutParam )
{
   bool success = true;
   StepResource pStep("Run Raster Math", "app", "{61D6A183-33F6-4fcd-AF99-009C46239D4F}");
   VERIFY(pStep.get());
   pStep->addProperty("name", getName());

   VERIFY(pInParam != NULL);
   mpProgress = pInParam->getPlugInArgValue<Progress>(ProgressArg());
   mpRaster = pInParam->getPlugInArgValue<RasterElement>(DataElementArg());

   RasterMathRunner runner(mpProgress, mAborted);

   if (isBatch())
   {
      try
      {
         success = batchExecute(pInParam, pOutParam, runner);
      }
      catch (RasterMathException& e)
      {
         pStep->addMessage(e.getMessage(), getName(), "{F3D07BB4-E866-4225-81FF-D1A9DA638C1A}");
         success = false;
      }
   }
   else
   {
      RasterMathDlgImp dialog(Service<DesktopServices>()->getMainWidget(), runner);
      if (dialog.exec() == QDialog::Rejected)
      {
         mAborted = true;
      }
   }

   if (!mAborted && mpProgress)
   {
      if (success)
      {
         mpProgress->updateProgress("Raster Math completed.", 100, NORMAL);
      }
      else
      {
         mpProgress->updateProgress("Raster Math failed.", 100, ERRORS);
      }
   }

   if (mAborted)
   {
      pStep->finalize(Message::Abort);
   }
   else
   {
      pStep->finalize(success?Message::Success:Message::Failure);
   }

   return success;
}

bool RasterMathPlugIn::batchExecute(PlugInArgList* pInParam, PlugInArgList* pOutParam, RasterMathRunner& runner)
{
   VERIFY(pOutParam != NULL);

   mResultsName = *RM_NULLCHK(pInParam->getPlugInArgValue<string>(RESULTS_NAME));
   mFormula = *RM_NULLCHK(pInParam->getPlugInArgValue<string>(FORMULA));
   mDisplayLayer = *RM_NULLCHK(pInParam->getPlugInArgValue<int>(DISPLAY_LAYER));
   mResultEncoding = *RM_NULLCHK(pInParam->getPlugInArgValue<EncodingType>(RESULT_ENCODING));
   double defaultValue = *RM_NULLCHK(pInParam->getPlugInArgValue<double>(DEFAULT_VALUE));
   bool failOnError = *RM_NULLCHK(pInParam->getPlugInArgValue<bool>(FAIL_ON_ERROR));
   bool radians = *RM_NULLCHK(pInParam->getPlugInArgValue<bool>(RADIANS));
   ProcessingLocation location = *RM_NULLCHK(pInParam->getPlugInArgValue<ProcessingLocation>(LOCATION));

   runner.setFailureMode(failOnError, defaultValue);
   runner.setBaseResultName(mResultsName);
   runner.setResultEncoding(mResultEncoding);
   runner.setRadians(radians);
   runner.setResultLocation(location);
   runner.setDisplayType(static_cast<RasterMathRunner::DisplayType>(mDisplayLayer));

   map<int,RasterElement*> rasterCorrelations;
   addCorrelation(rasterCorrelations, 1, *pInParam, DataElementArg());
   for (int i=2; i<=MAX_ARG; ++i)
   {
      addCorrelation(rasterCorrelations, i, *pInParam, RASTER_ARG+toString(i));
   }
   if (!rasterCorrelations.empty())
   {
      RM_NULLCHK(RasterCorrelator::instance())->setElements(rasterCorrelations);
   }

   map<int,AoiElement*> aoiCorrelations;
   for (int i=1; i<=MAX_ARG; ++i)
   {
      addCorrelation(aoiCorrelations, i, *pInParam, AOI_ARG+toString(i));
   }
   if (!aoiCorrelations.empty())
   {
      RM_NULLCHK(AoiCorrelator::instance())->setElements(aoiCorrelations);
   }

   runner.execute(mFormula);

   RasterElement* pRasterResult = runner.getRasterResult();
   Signature* pSignatureResult = runner.getSignatureResult();
   double scalarResult = runner.getScalarResult();
   if (pRasterResult != NULL)
   {
      VERIFY(pOutParam->setPlugInArgValue<RasterElement>(RASTER_RESULT, pRasterResult));
   }
   else if (pSignatureResult != NULL)
   {
      VERIFY(pOutParam->setPlugInArgValue<Signature>(SIGNATURE_RESULT, pSignatureResult));
   }
   else
   {
      VERIFY(pOutParam->setPlugInArgValue<double>(SCALAR_RESULT, &scalarResult));
   }
   return true;
}

bool RasterMathPlugIn::getInputSpecification(PlugInArgList*& pArgList)
{
   pArgList = Service<PlugInManagerServices>()->getPlugInArgList();
   VERIFY(pArgList != NULL);

   VERIFY(pArgList->addArg<Progress>(ProgressArg(), NULL));
   VERIFY(pArgList->addArg<SpatialDataView>(ViewArg(), NULL));
   VERIFY(pArgList->addArg<RasterElement>(DataElementArg(), NULL));

   if (isBatch())
   {
      VERIFY(pArgList->addArg<RasterElement>(RASTER2, NULL));
      VERIFY(pArgList->addArg<RasterElement>(RASTER3, NULL));
      VERIFY(pArgList->addArg<RasterElement>(RASTER4, NULL));
      VERIFY(pArgList->addArg<RasterElement>(RASTER5, NULL));
      VERIFY(pArgList->addArg<string>(FORMULA));
      VERIFY(pArgList->addArg<string>(RESULTS_NAME, &mResultsName));
      VERIFY(pArgList->addArg<int>(DISPLAY_LAYER, 0));
      VERIFY(pArgList->addArg<EncodingType>(RESULT_ENCODING, FLT4BYTES));
      VERIFY(pArgList->addArg<bool>(FAIL_ON_ERROR, false));
      VERIFY(pArgList->addArg<double>(DEFAULT_VALUE, 0.0));
      VERIFY(pArgList->addArg<bool>(RADIANS, true));
      VERIFY(pArgList->addArg<ProcessingLocation>(LOCATION, ProcessingLocation()));
      VERIFY(pArgList->addArg<RasterElement>(AOI1, NULL));
      VERIFY(pArgList->addArg<RasterElement>(AOI2, NULL));
      VERIFY(pArgList->addArg<RasterElement>(AOI3, NULL));
      VERIFY(pArgList->addArg<RasterElement>(AOI4, NULL));
      VERIFY(pArgList->addArg<RasterElement>(AOI5, NULL));
   }

   return true;
}

bool RasterMathPlugIn::getOutputSpecification(PlugInArgList*& pArgList)
{
   pArgList = Service<PlugInManagerServices>()->getPlugInArgList();
   VERIFY(pArgList != NULL);

   if (isBatch())
   {
      VERIFY(pArgList->addArg<double>(SCALAR_RESULT));
      VERIFY(pArgList->addArg<Signature>(SIGNATURE_RESULT));
      VERIFY(pArgList->addArg<RasterElement>(RASTER_RESULT));
   }

   return true;
}
