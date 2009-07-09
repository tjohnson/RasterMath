/*
 * The information in this file is
 * Copyright(c) 2007 Ball Aerospace & Technologies Corporation
 * and is subject to the terms and conditions of the
 * GNU Lesser General Public License Version 2.1
 * The license text is available from   
 * http://www.gnu.org/licenses/lgpl.html
 */

#include "AoiElement.h"
#include "AppVerify.h"
#include "ModelServices.h"
#include "RasterCorrelator.h"
#include "RasterElement.h"
#include "RasterMathDlgImp.h"
#include "RasterMathException.h"
#include "RasterMathRunner.h"
#include "TypeConverter.h"

#include <QtGui/QInputDialog>
#include <QtGui/QMessageBox>
#include <QtGui/QPushButton>

#include <algorithm>

using namespace std;

namespace
{
   const QString defaultResultName = "Raster Math Result";

   template<class T>
   int getCorrelatorIndex(const vector<DataElement*>& allElements, int index)
   {
      DataElement* pElement = RM_NULLCHK(Correlator<T>::instance())->getElement(index);
      for (unsigned int i=0; i<allElements.size(); ++i)
      {
         if (pElement == allElements[i])
         {
            return i;
         }
      }
      return 0;
   }

   template <class T>
   void populateCombos(vector<QComboBox*>& ppCombos)
   {
      vector<DataElement*> allElements = Service<ModelServices>()->getElements(TypeConverter::toString<T>());
      QStringList names;
      for (vector<DataElement*>::iterator ppElement=allElements.begin(); ppElement!=allElements.end(); ++ppElement)
      {
         names.append(QString::fromStdString(RM_NULLCHK(*ppElement)->getName()));
      }

      for (int i=0; i<5; ++i)
      {
         ppCombos[i]->clear();
         ppCombos[i]->addItems(names);
         ppCombos[i]->setCurrentIndex(getCorrelatorIndex<T>(allElements, i+1));
      }
   }

   template <class T>
   void setCorrelations(const vector<QComboBox*>& ppCombos)
   {
      vector<DataElement*> allElements = Service<ModelServices>()->getElements(TypeConverter::toString<T>());

      map<int,T*> elements;
      if (allElements.empty() == false)
      {
         for (int i=0; i<5; ++i)
         {
            elements[i+1] = RM_NULLCHK(dynamic_cast<T*>(allElements[ppCombos[i]->currentIndex()]));
         }
         
         RM_NULLCHK(Correlator<T>::instance())->setElements(elements);
      }
   }

   template<class T>
   class VariableResetter
   {
   public:
      VariableResetter(T& theVariable) : mVariableRef(theVariable), mVariableVal(theVariable) {}
      ~VariableResetter() { mVariableRef = mVariableVal; }
   private:
      T& mVariableRef;
      T mVariableVal;
   };
}

#define RECURSION_GUARD \
   static bool rgBusy = false; \
   if (rgBusy) \
   { \
      return; \
   } \
   VariableResetter<bool> rgResetter(rgBusy); \
   rgBusy = true;

RasterMathDlgImp::RasterMathDlgImp(QWidget *pParent, RasterMathRunner& runner) :
   QDialog(pParent),
   mRunner(runner),
   mNeedsRun(false),
   mValidator(NULL)
{
   setupUi(this);

   mpAoiCombos.push_back(mpAoi1Combo);
   mpAoiCombos.push_back(mpAoi2Combo);
   mpAoiCombos.push_back(mpAoi3Combo);
   mpAoiCombos.push_back(mpAoi4Combo);
   mpAoiCombos.push_back(mpAoi5Combo);

   mpRasterCombos.push_back(mpRaster1Combo);
   mpRasterCombos.push_back(mpRaster2Combo);
   mpRasterCombos.push_back(mpRaster3Combo);
   mpRasterCombos.push_back(mpRaster4Combo);
   mpRasterCombos.push_back(mpRaster5Combo);

   populateCombos<AoiElement>(mpAoiCombos);
   populateCombos<RasterElement>(mpRasterCombos);
   populateFavorites();
   needsRun(true);

   mpResultNameTextEdit->setText(defaultResultName);

   mpErrorUseTextEdit->setValidator(&mValidator);

   VERIFYNRV(connect(mpAddToFavoritesButton, SIGNAL(clicked()), this, SLOT(addToFavorites())));
   VERIFYNRV(connect(mpDeleteFavoriteButton, SIGNAL(clicked()), this, SLOT(deleteFavorite())));
   VERIFYNRV(connect(mpButtonBox, SIGNAL(clicked (QAbstractButton*)), this, SLOT(apply(QAbstractButton*))));
   VERIFYNRV(connect(mpFavoritesCombo, SIGNAL(activated (const QString &)), this, SLOT(favoriteSelected(const QString &))));

   VERIFYNRV(connect(mpFavoritesCombo, SIGNAL(activated (const QString &)), this, SLOT(needsRun())));
   VERIFYNRV(connect(mpDisplayAsCombo, SIGNAL(activated (const QString &)), this, SLOT(needsRun())));
   VERIFYNRV(connect(mpFormulaTextEdit, SIGNAL(textChanged ()), this, SLOT(needsRun())));
   VERIFYNRV(connect(mpFormulaTextEdit, SIGNAL(textChanged ()), this, SLOT(toNoFavorite())));
   VERIFYNRV(connect(mpResultNameTextEdit, SIGNAL(textChanged (const QString &)), this, SLOT(needsRun())));
   VERIFYNRV(connect(mpRadiansButton, SIGNAL(toggled(bool)), this, SLOT(needsRun())));
   VERIFYNRV(connect(mpDegreesButton, SIGNAL(toggled(bool)), this, SLOT(needsRun())));
   VERIFYNRV(connect(mpErrorFailButton, SIGNAL(toggled(bool)), this, SLOT(needsRun())));
   VERIFYNRV(connect(mpErrorUseButton, SIGNAL(toggled(bool)), this, SLOT(needsRun())));
   VERIFYNRV(connect(mpErrorUseTextEdit, SIGNAL(textChanged (const QString &)), this, SLOT(needsRun())));

   for (int i=0; i<5; i++)
   {
      VERIFYNRV(connect(mpAoiCombos[i], SIGNAL(activated (const QString &)), this, SLOT(needsRun())));
      VERIFYNRV(connect(mpRasterCombos[i], SIGNAL(activated (const QString &)), this, SLOT(needsRun())));
   }
}

void RasterMathDlgImp::accept()
{
   RECURSION_GUARD;
   try
   {
      if (mNeedsRun)
      {
         executeRunner();
      }
      QDialog::accept();
   }
   catch (RasterMathAbortException&)
   {
   }
   catch (RasterMathException& e)
   {
      if (QMessageBox::critical(this, "Raster Math failed", QString::fromStdString(e.getMessage()), "Retry", "Cancel") == 1)
      {
         QDialog::reject();
      }
   }
}

void RasterMathDlgImp::reject()
{
   RECURSION_GUARD;
   QDialog::reject();
}

string RasterMathDlgImp::getFormula() const
{
   return mpFormulaTextEdit->toPlainText().toStdString();
}

void RasterMathDlgImp::populateFavorites()
{
   mpFavoritesCombo->clear();
   mpFavoritesCombo->addItem(QString());
   if (hasSettingFavoriteNames())
   {
      vector<string> favoriteNames = getSettingFavoriteNames();
      for (vector<string>::iterator pNode=favoriteNames.begin(); pNode != favoriteNames.end(); ++pNode)
      {
         mpFavoritesCombo->addItem(QString::fromStdString(*pNode));
      }
   }
}

void RasterMathDlgImp::addToFavorites()
{
   RECURSION_GUARD;
   bool done = false;
   while (!done)
   {
      bool ok = true;
      QString name = QInputDialog::getText(this, "Add to Favorites", "Enter the name for the new favorite:", QLineEdit::Normal, QString(), &ok);
      if (ok)
      {
         if (name.isEmpty())
         {
            if (QMessageBox::critical(this, "Add to Favorites", "You must enter a name for the favorite", QMessageBox::Retry, QMessageBox::Cancel) == QMessageBox::Cancel)
            {
               done = true;
            }
         }
         else
         {
            vector<string> favoriteNames;
            if (hasSettingFavoriteNames())
            {
               favoriteNames = getSettingFavoriteNames();
            }
            if (std::find(favoriteNames.begin(), favoriteNames.end(), name.toStdString()) != favoriteNames.end())
            {
               if (QMessageBox::critical(this, "Add to Favorites", "A favorite with the specified name already exists", QMessageBox::Retry, QMessageBox::Cancel) == QMessageBox::Cancel)
               {
                  done = true;
               }
            }
            else
            {
               favoriteNames.push_back(name.toStdString());
               vector<string> favoriteFormulas = getSettingFavoriteFormulas();
               favoriteFormulas.push_back(mpFormulaTextEdit->toPlainText().toStdString());
               setSettingFavoriteNames(favoriteNames);
               setSettingFavoriteFormulas(favoriteFormulas);
               populateFavorites();
               mpFavoritesCombo->setCurrentIndex(mpFavoritesCombo->findText(name));
               done = true;
            }
         }
      }
      else
      {
         done = true;
      }
   }
}

void RasterMathDlgImp::deleteFavorite()
{
   RECURSION_GUARD;
   if (mpFavoritesCombo->currentIndex() == 0)
   {
      QMessageBox::warning(this, "Delete Favorite", "No favorite is currently selected");
      return;
   }

   if (QMessageBox::information(this, "Delete Favorite", "Do you want to delete the current favorite?", QMessageBox::Ok, QMessageBox::Cancel) == QMessageBox::Cancel)
   {
      return;
   }

   QString name = mpFavoritesCombo->currentText();
   if (hasSettingFavoriteNames())
   {
      vector<string> favoriteNames = getSettingFavoriteNames();
      vector<string>::iterator pName = std::find(favoriteNames.begin(), favoriteNames.end(), name.toStdString());
      VERIFYNRV(pName != favoriteNames.end());
      vector<string> favoriteFormulas = getSettingFavoriteFormulas();
      favoriteFormulas.erase(favoriteFormulas.begin()+(pName-favoriteNames.begin()));
      favoriteNames.erase(pName);
      setSettingFavoriteNames(favoriteNames);
      setSettingFavoriteFormulas(favoriteFormulas);
      populateFavorites();
      mpFavoritesCombo->setCurrentIndex(0);
      mpResultNameTextEdit->setText(defaultResultName);
   }
}

void RasterMathDlgImp::favoriteSelected(const QString &name)
{
   if (name.isEmpty())
   {
      mpResultNameTextEdit->setText(defaultResultName);
      return;
   }
   if (hasSettingFavoriteNames())
   {
      vector<string> favoriteNames = getSettingFavoriteNames();
      vector<string>::iterator pName = std::find(favoriteNames.begin(), favoriteNames.end(), name.toStdString());
      VERIFYNRV(pName != favoriteNames.end());
      vector<string> favoriteFormulas = getSettingFavoriteFormulas();
      string formula = favoriteFormulas[pName-favoriteNames.begin()];
      VERIFYNRV(disconnect(mpFormulaTextEdit, SIGNAL(textChanged ()), this, SLOT(toNoFavorite())));
      mpFormulaTextEdit->setText(QString::fromStdString(formula));
      VERIFYNRV(connect(mpFormulaTextEdit, SIGNAL(textChanged ()), this, SLOT(toNoFavorite())));
      mpResultNameTextEdit->setText(QString::fromStdString(*pName));
   }
}

void RasterMathDlgImp::apply(QAbstractButton* pButton)
{
   RECURSION_GUARD;
   if (mpButtonBox->buttonRole(pButton) != QDialogButtonBox::ApplyRole)
   {
      return;
   }

   try
   {
      executeRunner();
      populateCombos<AoiElement>(mpAoiCombos);
      populateCombos<RasterElement>(mpRasterCombos);
   }
   catch (RasterMathAbortException&)
   {
   }
   catch (RasterMathException& e)
   {
      QMessageBox::critical(this, "Raster Math failed", QString::fromStdString(e.getMessage()));
   }
}

void RasterMathDlgImp::executeRunner()
{
   setCorrelations<AoiElement>(mpAoiCombos);
   setCorrelations<RasterElement>(mpRasterCombos);
   mRunner.setDisplayType(static_cast<RasterMathRunner::DisplayType>(mpDisplayAsCombo->currentIndex()));
   mRunner.setBaseResultName(mpResultNameTextEdit->text().toStdString());
   map<int,EncodingType> index2Encoding;
   index2Encoding[0] = INT1SBYTE;
   index2Encoding[1] = INT1UBYTE;
   index2Encoding[2] = INT2SBYTES;
   index2Encoding[3] = INT2UBYTES;
   index2Encoding[4] = INT4SBYTES;
   index2Encoding[5] = INT4UBYTES;
   index2Encoding[6] = FLT4BYTES;
   index2Encoding[7] = FLT8BYTES;
   mRunner.setResultEncoding(index2Encoding[mpPrecisionCombo->currentIndex()]);
   mRunner.setFailureMode(mpErrorFailButton->isChecked(), mpErrorUseTextEdit->text().toDouble());
   mRunner.setRadians(mpRadiansButton->isChecked());
   int locationIndex = mpLocationCombo->currentIndex();
   ProcessingLocation pl;
   map<int,ProcessingLocation> index2Location;
   index2Location[0] = ON_DISK;
   index2Location[1] = IN_MEMORY;
   index2Location[2] = pl;
   mRunner.setResultLocation(index2Location[locationIndex]);
   mRunner.execute(getFormula());
   needsRun(false);
}

void RasterMathDlgImp::needsRun(bool run)
{
   if (mNeedsRun != run)
   {
      mNeedsRun = run;
      QPushButton* pOkButton = RM_NULLCHK(mpButtonBox->button(QDialogButtonBox::Ok));
      QPushButton* pCancelButton = RM_NULLCHK(mpButtonBox->button(QDialogButtonBox::Cancel));
      if (run)
      {
         pOkButton->setEnabled(true);
         pCancelButton->setText("Cancel");
      }
      else
      {
         pOkButton->setEnabled(false);
         pCancelButton->setText("Close");
      }
   }
}

void RasterMathDlgImp::toNoFavorite()
{
   mpFavoritesCombo->setCurrentIndex(0);
   mpResultNameTextEdit->setText(defaultResultName);
}
