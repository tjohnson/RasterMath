/*
 * The information in this file is
 * Copyright(c) 2007 Ball Aerospace & Technologies Corporation
 * and is subject to the terms and conditions of the
 * GNU Lesser General Public License Version 2.1
 * The license text is available from   
 * http://www.gnu.org/licenses/lgpl.html
 */

#include "AoiElement.h"
#include "DesktopServices.h"
#include "LayerList.h"
#include "ModelServices.h"
#include "RasterCorrelator.h"
#include "RasterElement.h"
#include "RasterMathException.h"
#include "SpatialDataView.h"

using namespace std;

template class Correlator<RasterElement>;
template class Correlator<AoiElement>;

Correlator<RasterElement>* Correlator<RasterElement>::spInstance = NULL;
Correlator<AoiElement>* Correlator<AoiElement>::spInstance = NULL;

template<>
Correlator<RasterElement>::Correlator()
{
   vector<DataElement*> allElements = Service<ModelServices>()->getElements("RasterElement");

   RasterElement* pPrimary = NULL;
   SpatialDataView* pView = dynamic_cast<SpatialDataView*>(Service<DesktopServices>()->getCurrentWorkspaceWindowView());
   if (pView != NULL)
   {
      pPrimary = RM_NULLCHK(pView->getLayerList())->getPrimaryRasterElement();
   }

   int index = 1;
   for (vector<DataElement*>::iterator ppElement=allElements.begin(); ppElement!=allElements.end(); ++ppElement)
   {
      RasterElement* pElement = dynamic_cast<RasterElement*>(*ppElement);
      if (pElement == pPrimary)
      {
         mElements[index++] = pPrimary;
         break;
      }
   }

   init(allElements, index, pPrimary);
}

template<>
Correlator<AoiElement>::Correlator()
{
   vector<DataElement*> allElements = Service<ModelServices>()->getElements("AoiElement");
   init(allElements, 1, NULL);
}

template<class T>
void Correlator<T>::init(const vector<DataElement*>& allElements, int startIndex, T* pPrimary)
{
   int index = startIndex;
   for (vector<DataElement*>::const_iterator ppElement=allElements.begin(); ppElement!=allElements.end()&&index<=MAX_CORREL; ++ppElement)
   {
      T* pElement = dynamic_cast<T*>(*ppElement);
      if (pElement != pPrimary)
      {
         mElements[index++] = pElement;
      }
   }
}

template<class T>
T* Correlator<T>::getElement(int index)
{
   map<int,T*>::iterator pNode = mElements.find(index);
   if (pNode != mElements.end())
   {
      return pNode->second;
   }
   else
   {
      return NULL;
   }
}

template<class T>
void Correlator<T>::setElements(const map<int,T*>& elements)
{
   mElements = elements;
}

template<class T>
void Correlator<T>::setResultElement(T *pElement)
{
   mElements[0] = pElement;
}

/*
RasterCorrelator* RasterCorrelator::spInstance = NULL;

RasterCorrelator::RasterCorrelator()
{
   RasterElement* pPrimary = NULL;
   SpatialDataView* pView = dynamic_cast<SpatialDataView*>(Service<DesktopServices>()->getCurrentWorkspaceWindowView());
   if (pView != NULL)
   {
      pPrimary = RM_NULLCHK(pView->getLayerList())->getPrimaryRasterElement();
   }

   vector<DataElement*> allElements = Service<ModelServices>()->getElements("RasterElement");

   int index = 1;
   for (vector<DataElement*>::iterator ppElement=allElements.begin(); ppElement!=allElements.end(); ++ppElement)
   {
      RasterElement* pElement = dynamic_cast<RasterElement*>(*ppElement);
      if (pElement == pPrimary)
      {
         mElements[index++] = pPrimary;
         break;
      }
   }

   for (vector<DataElement*>::iterator ppElement=allElements.begin(); ppElement!=allElements.end()&&index<=MAX_CORREL; ++ppElement)
   {
      RasterElement* pElement = dynamic_cast<RasterElement*>(*ppElement);
      if (pElement != pPrimary)
      {
         mElements[index++] = pElement;
      }
   }
}

RasterElement* RasterCorrelator::getElement(int index)
{
   map<int,RasterElement*>::iterator pNode = mElements.find(index);
   if (pNode != mElements.end())
   {
      return pNode->second;
   }
   else
   {
      return NULL;
   }
}

void RasterCorrelator::setElements(const map<int,RasterElement*>& elements)
{
   mElements = elements;
}

void RasterCorrelator::setResultElement(RasterElement *pElement)
{
   mElements[0] = pElement;
}
*/