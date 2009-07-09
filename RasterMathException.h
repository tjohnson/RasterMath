/*
 * The information in this file is
 * Copyright(c) 2009 Todd A. Johnson
 * and is subject to the terms and conditions of the
 * GNU Lesser General Public License Version 2.1
 * The license text is available from   
 * http://www.gnu.org/licenses/lgpl.html
 */

#ifndef RASTERMATHEXCEPTION_H
#define RASTERMATHEXCEPTION_H

#include <exception>
#include <string>

class RasterMathException : public std::exception
{
public:
   RasterMathException(const std::string& message) : mMessage(message) {}
   const std::string& getMessage() const { return mMessage; }
   static std::string defaultMessage(const std::string& prefix, const char* pFileName, int line);
private:
   std::string mMessage;
};

class RasterMathAbortException : public RasterMathException
{
public:
   RasterMathAbortException(const std::string& message) : RasterMathException(message) {}
};

template<class T> T* rasterMathNullChecker(T* pT, const char* pFileName, int line)
{
   if (!pT)
   {
      throw RasterMathException(RasterMathException::defaultMessage("Null Pointer Reference", pFileName, line));
   }
   return pT;
}
template<class T> T& rasterMathNullChecker(T& rT, const char* pFileName, int line)
{
   if (!rT)
   {
      throw RasterMathException(RasterMathException::defaultMessage("Null Pointer Reference", pFileName, line));
   }
   return rT;
}

#define RM_VERIFY(x)\
   ((x)?0:throw RasterMathException(RasterMathException::defaultMessage(#x, __FILE__, __LINE__)))

#define RM_NULLCHK(x) rasterMathNullChecker(x, __FILE__, __LINE__)

#endif
