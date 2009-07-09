/*
 * The information in this file is
 * Copyright(c) 2009 Todd A. Johnson
 * and is subject to the terms and conditions of the
 * GNU Lesser General Public License Version 2.1
 * The license text is available from   
 * http://www.gnu.org/licenses/lgpl.html
 */

#ifndef RASTERCORRELATOR_H
#define RASTERCORRELATOR_H

class AoiElement;
class DataElement;
class RasterElement;

#include <map>
#include <vector>

template<class T>
class Correlator
{
public:
   static const int MAX_CORREL = 9;

   static Correlator<T>* instance()
   {
      if (spInstance == NULL)
      {
         spInstance = new Correlator<T>;
      }
      return spInstance;
   }

   T* getElement(int index);
   void setElements(const std::map<int,T*>& elements);

   void setResultElement(T* pElement);

private:
   Correlator();
   void init(const std::vector<DataElement*>& allElements, int startIndex, T* pPrimary);

   static Correlator<T>* spInstance;
   std::map<int,T*> mElements;
};

typedef Correlator<RasterElement> RasterCorrelator;
typedef Correlator<AoiElement> AoiCorrelator;

#endif
