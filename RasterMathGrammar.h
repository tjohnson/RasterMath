/*
 * The information in this file is
 * Copyright(c) 2007 Ball Aerospace & Technologies Corporation
 * and is subject to the terms and conditions of the
 * GNU Lesser General Public License Version 2.1
 * The license text is available from   
 * http://www.gnu.org/licenses/lgpl.html
 */

#ifndef RASTERMATHGRAMMAR_H
#define RASTERMATHGRAMMAR_H

#include <boost/spirit/core.hpp>
#include <iostream>
#include <string>

struct RmGrammar : public boost::spirit::grammar<RmGrammar>
 {
     template <typename ScannerT>
     struct definition
     {
         definition(RmGrammar const& self);

         boost::spirit::rule<ScannerT> fullexpr, expr8, expr7, expr6, expr5, expr4, expr3, expr2, expr1, 
            not, conjunction, group, number, constant, func3, func2, func1, statfunc1, function, 
            refName, ref1, ref2, ref3, ref4, ref5, fullref, integer, aoiName;

         boost::spirit::rule<ScannerT> const& start() const 
         { 
            return fullexpr; 
         }
     };
 };

#endif
