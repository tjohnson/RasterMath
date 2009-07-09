/*
 * The information in this file is
 * Copyright(c) 2007 Ball Aerospace & Technologies Corporation
 * and is subject to the terms and conditions of the
 * GNU Lesser General Public License Version 2.1
 * The license text is available from   
 * http://www.gnu.org/licenses/lgpl.html
 */

#ifndef PARSESTACKBUILDER_H
#define PARSESTACKBUILDER_H

#include <string>
#include <vector>

class ProcessStack;

class ParseStackBuilder
{
public:
   static void clear();
   static ProcessStack& getStack();

   static void integer(int d);
   static void number(double d);
   static void pi(char const* pStr, char const* pEnd);
   static void e(char const* pStr, char const* pEnd);
   static void negate(char const* pStr, char const* pEnd);
   static void add(char const* pStr, char const* pEnd);
   static void subtract(char const* pStr, char const* pEnd);
   static void multiply(char const* pStr, char const* pEnd);
   static void divide(char const* pStr, char const* pEnd);
   static void modulo(char const* pStr, char const* pEnd);
   static void exponentiate(char const* pStr, char const* pEnd);
   static void equals(char const* pStr, char const* pEnd);
   static void notEquals(char const* pStr, char const* pEnd);
   static void lessThan(char const* pStr, char const* pEnd);
   static void greaterThan(char const* pStr, char const* pEnd);
   static void lessOrEqual(char const* pStr, char const* pEnd);
   static void greaterOrEqual(char const* pStr, char const* pEnd);
   static void not(char const* pStr, char const* pEnd);
   static void and(char const* pStr, char const* pEnd);
   static void or(char const* pStr, char const* pEnd);
   static void func1(char const* pStr, char const* pEnd);
   static void func2(char const* pStr, char const* pEnd);
   static void func3(char const* pStr, char const* pEnd);
   static void statfunc1(char const* pStr, char const* pEnd);
   static void fullRaster(char const* pStr, char const* pEnd);
   static void rasterIndex(char const* pStr, char const* pEnd);
   static void rasterFullSlice(char const* pStr, char const* pEnd);
   static void rasterNtoEndSlice(char const* pStr, char const* pEnd);
   static void raster0toNSlice(char const* pStr, char const* pEnd);
   static void aoi(char const* pStr, char const* pEnd);
};

#endif
