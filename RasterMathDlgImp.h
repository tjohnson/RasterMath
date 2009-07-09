/*
 * The information in this file is
 * Copyright(c) 2007 Ball Aerospace & Technologies Corporation
 * and is subject to the terms and conditions of the
 * GNU Lesser General Public License Version 2.1
 * The license text is available from   
 * http://www.gnu.org/licenses/lgpl.html
 */

#ifndef RASTER_MATH_DLG_IMP
#define RASTER_MATH_DLG_IMP

#include "ConfigurationSettings.h"
#include "ui_RasterMathDlg.h"

#include <QtGui/QDialog>

#include <string>
#include <vector>

class RasterMathRunner;

class RasterMathDlgImp : public QDialog, public Ui::RasterMathDlg
{
   Q_OBJECT

   // would have been much cleaner as map<string,string>, but that can't go into a DataVariant
   SETTING(FavoriteFormulas, RasterMathDlg, std::vector<std::string>, std::vector<std::string>());
   SETTING(FavoriteNames, RasterMathDlg, std::vector<std::string>, std::vector<std::string>());

public:
   RasterMathDlgImp(QWidget *pParent, RasterMathRunner& runner);

   std::string getFormula() const;

public slots:
   void accept();
   void reject();

private slots:
   void addToFavorites();
   void deleteFavorite();
   void apply(QAbstractButton*);
   void favoriteSelected(const QString &name);
   void needsRun(bool run=true);
   void toNoFavorite();

private:
   void populateFavorites();
   void executeRunner();

   RasterMathRunner& mRunner;
   bool mNeedsRun;
   QDoubleValidator mValidator;
   std::vector<QComboBox*> mpAoiCombos;
   std::vector<QComboBox*> mpRasterCombos;
};

#endif
