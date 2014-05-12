/*****************************************************************************
 * productsmodel.cpp
 *
 * Created: 22/04/2014 by vijay
 *
 * Copyright 2014 ThamiZha!. All rights reserved.
 *
 * Visit www.thamizha.com for more information.
 *
 * This file is a part of ThamiZha Angadi application.
 *
 * This file may be distributed under the terms of GNU Public License version
 * 3 (GPL v3) as defined by the Free Software Foundation (FSF). A copy of the
 * license should have been included with this file, or the project in which
 * this file belongs to. You may also find the details of GPL v3 at:
 * http://www.gnu.org/licenses/gpl-3.0.txt
 *
 * If you have any questions regarding the use of this file, feel free to
 * contact the author of this file, or the owner of the project in which
 * this file belongs to.
 *
 * Authors :
 * D.Mohan Raj <mohanraj.hunk@live.com>
 *****************************************************************************/

#include "transactionform.h"
#include "ui_transactionform.h"

TransactionForm::TransactionForm(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::TransactionForm)
{
    ui->setupUi(this);

    formValidation = new FormValidation;

    dataMapper = new QDataWidgetMapper(this);
    dataMapper->setItemDelegate(new QSqlRelationalDelegate(this));
    dataMapper->setSubmitPolicy(QDataWidgetMapper::ManualSubmit);

    ui->pushButtonDelete->setEnabled(false);

    ui->comboBoxPayMode->addItem("Cash");
    ui->comboBoxPayMode->addItem("DD");
    ui->comboBoxPayMode->addItem("Cheque");

    connect(ui->pushButtonSave,SIGNAL(clicked()),this,SLOT(save()));
    connect(ui->lineEditInvoiceNo,SIGNAL(textChanged(QString)),this,SLOT(onNameChanged(QString)));
    connect(ui->lineEditInvoiceNo,SIGNAL(returnPressed()),this,SLOT(setSignalFromTransactionForm()));

    connect(ui->lineEditInvoiceNo,SIGNAL(editingFinished()),SLOT(invoiceNoValid()));
    connect(ui->lineEditPaidAmount,SIGNAL(editingFinished()),SLOT(amountValid()));

    generateDate();
    setFieldMaxLength();
}

TransactionForm::~TransactionForm()
{
    delete ui;
}

void TransactionForm::setFieldMaxLength()
{
    ui->lineEditPaidAmount->setMaxLength(10);
    ui->lineEditPaidBy->setMaxLength(150);
}

void TransactionForm::generateDate()
{
    QDate currentDate = QDate::currentDate();
    ui->dateEditPaymentDate->setDate(currentDate);
}

void TransactionForm::save()
{
    // Initialization of local variables
    int validError = 0;
    QString errors = "";

    // Initialization of message box
    QMessageBox msgBox;
    msgBox.setText("Validation Error in this forms. Please correct the form and resubmit it");
    msgBox.setInformativeText("");
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.setDefaultButton(QMessageBox::Ok);

    // validate code field
    if(!TransactionForm::invoiceNoValid()){
        validError = 1;
        errors.append("\nThe Invoice No field may be empty or already exist");
    }

    // Validate Name field
    if(!TransactionForm::amountValid()){
        validError = 1;
        errors.append("\nThe Paid amount field may be empty or already exist");
    }

    // save the form if there is no errors
    if(validError == 0){
        bool status;

        if(dataMapper->currentIndex() < 0){
            int row = transactionModel->rowCount();
            transactionModel->insertRows(row, 1);

            transactionModel->setData(transactionModel->index(row,transactionModel->fieldIndex("paidAmount")),ui->lineEditPaidAmount->text());
            transactionModel->setData(transactionModel->index(row,transactionModel->fieldIndex("paidBy")),ui->lineEditPaidBy->text());

            QSqlQueryModel model;
            QSqlQuery query;
            query.prepare("Select * from bill where invoiceNo = :invoiceNo");
            query.bindValue(":invoiceNo", ui->lineEditInvoiceNo->text());
            query.exec();
            model.setQuery(query);
            QSqlRecord record = model.record(0);
            int bill_id = record.value("id").toInt();
            int customer_id = record.value("customer_id").toInt();

            QModelIndex idx = transactionModel->index(row,1);
            transactionModel->setData(idx,bill_id,Qt::EditRole);
            QModelIndex idc = transactionModel->index(row,2);
            transactionModel->setData(idc,customer_id,Qt::EditRole);

            transactionModel->setData(transactionModel->index(row,transactionModel->fieldIndex("mode")),ui->comboBoxPayMode->currentText());

            QDateTime datetime = QDateTime::currentDateTime();
            transactionModel->setData(transactionModel->index(row,transactionModel->fieldIndex("createdDate")),datetime.toString("yyyy-MM-dd hh:mm:ss"));

            QDate payDate = ui->dateEditPaymentDate->date();
            transactionModel->setData(transactionModel->index(row,transactionModel->fieldIndex("paidOn")),payDate.toString("yyyy-MM-dd"));

            transactionModel->setData(transactionModel->index(row,transactionModel->fieldIndex("status")),"A");

            transactionModel->submitAll();

            statusMsg = ui->lineEditInvoiceNo->text() + " saved successfully";
            emit signalStatusBar(statusMsg);
            emit signalUpdated();
        }else{
            QDateTime datetime = QDateTime::currentDateTime();

            QSqlQueryModel model;
            QSqlQuery query;
            query.prepare("Select * from bill where invoiceNo = :bill_no");
            query.bindValue(":bill_no", ui->lineEditInvoiceNo->text());
            query.exec();
            model.setQuery(query);
            QSqlRecord record = model.record(0);
            int bill_id = record.value("id").toInt();
            int customer_id = record.value("customer_id").toInt();

            QModelIndex idx = transactionModel->index(dataMapper->currentIndex(),1);
            transactionModel->setData(idx, bill_id, Qt::EditRole);
            status = dataMapper->submit();
            QModelIndex idc = transactionModel->index(dataMapper->currentIndex(),2);
            transactionModel->setData(idc,customer_id,Qt::EditRole);
            transactionModel->setData(transactionModel->index(dataMapper->currentIndex(),transactionModel->fieldIndex("modifiedDate")),datetime);

            if(status == true){
                transactionModel->submitAll();
                statusMsg = ui->lineEditInvoiceNo->text() + " updated successfully";
                emit signalStatusBar(statusMsg);
            }
            emit signalUpdated();
        }

        resetDataMapper();
        clear();
        setCodeFocus();

    }else{
        msgBox.setInformativeText(errors);
        int ret = msgBox.exec();
        switch (ret) {
           case QMessageBox::Ok:
               ui->lineEditInvoiceNo->setFocus();
               ui->lineEditInvoiceNo->selectAll();
               break;
           default:
               // should never be reached
               break;
         }
    }
}

void TransactionForm::clear()
{
    foreach(QLineEdit *widget, this->findChildren<QLineEdit*>()) {
        widget->clear();
        widget->setProperty("validationError",false);
        widget->setProperty("validationSuccess",false);
        widget->setStyleSheet(styleSheet());
    }
    ui->comboBoxPayMode->setCurrentIndex(0);
    uninstallEventFilter();
    ui->pushButtonSave->setText("Save");
    ui->pushButtonDelete->setEnabled(false);

}

void TransactionForm::setModel(TransactionModel *model)
{
    transactionModel = model;

    dataMapper->setModel(transactionModel);
    dataMapper->addMapping(ui->dateEditPaymentDate,transactionModel->fieldIndex("paidOn"));
    dataMapper->addMapping(ui->lineEditInvoiceNo,transactionModel->relationModel(1)->fieldIndex("invoiceNo"));
    dataMapper->addMapping(ui->lineEditPaidAmount,transactionModel->fieldIndex("paidAmount"));
    dataMapper->addMapping(ui->lineEditPaidBy,transactionModel->fieldIndex("paidBy"));
    dataMapper->addMapping(ui->comboBoxPayMode,transactionModel->fieldIndex("mode"));

    setCodeFocus();
}

void TransactionForm::setMapperIndex(QModelIndex index)
{
    clear();
    dataMapper->setCurrentIndex(index.row());
    ui->pushButtonSave->setText("Update");
    ui->pushButtonDelete->setEnabled(true);
}

void TransactionForm::setCodeFocus()
{
    ui->lineEditInvoiceNo->setFocus();
    ui->lineEditInvoiceNo->selectAll();
}

void TransactionForm::setNameFocus()
{
    ui->lineEditInvoiceNo->setFocus();
    ui->lineEditInvoiceNo->selectAll();
}

void TransactionForm::search(QString value)
{
    QString searchValue = "bill_id = ";
    searchValue.append(value);
    transactionModel->selectRow(1);
}

void TransactionForm::on_pushButtonCancel_clicked()
{
    resetDataMapper();
    clear();
    setCodeFocus();
}

void TransactionForm::on_pushButtonDelete_clicked()
{
    QSqlRecord record = transactionModel->record(dataMapper->currentIndex());
    statusMsg = ui->lineEditInvoiceNo->text() + " deleted successfully";

    QDateTime datetime = QDateTime::currentDateTime();
    QChar t_status = 'D';
    record.setValue("status", t_status);
    record.setValue("modifiedDate", datetime);
    transactionModel->setRecord(dataMapper->currentIndex(),record);
    transactionModel->submitAll();
    transactionModel->select();

    emit signalStatusBar(statusMsg);
    emit signalUpdated();
    on_pushButtonCancel_clicked();
}

void TransactionForm::onNameChanged(QString str)
{
    emit signalName(str);
}

void TransactionForm::setSignalFromTransactionForm()
{
    emit signalFromTransactionForm();
}

void TransactionForm::resetDataMapper()
{
    dataMapper = new QDataWidgetMapper(this);
    dataMapper->setSubmitPolicy(QDataWidgetMapper::ManualSubmit);
    setModel(transactionModel);
}

bool TransactionForm::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == ui->lineEditInvoiceNo){
        if (event->type() == QEvent::FocusIn)
            TransactionForm::invoiceNoValid();
        return false;
    }else if (obj == ui->lineEditPaidAmount){
        if (event->type() == QEvent::FocusIn)
            TransactionForm::amountValid();
        return false;
    }
    return TransactionForm::eventFilter(obj, event);
}

void TransactionForm::uninstallEventFilter()
{
    ui->lineEditInvoiceNo->removeEventFilter(this);
    ui->lineEditPaidAmount->removeEventFilter(this);
    ui->flashMsgUp->clear();
}

void TransactionForm::setAllValidationSuccess()
{
    foreach(QLineEdit *widget, this->findChildren<QLineEdit*>()) {
        widget->setProperty("validationError",false);
        widget->setProperty("validationSuccess",false);
        widget->setStyleSheet(styleSheet());
    }
}

bool TransactionForm::invoiceNoValid()
{
    bool status = false;
    QString flashMsg = "";
    ui->lineEditInvoiceNo->installEventFilter(this);
    if(ui->lineEditInvoiceNo->text().length() > 0){
        if (billStatus()){
            ui->lineEditInvoiceNo->setProperty("validationError",false);
            ui->lineEditInvoiceNo->setProperty("validationSuccess",true);
            ui->lineEditInvoiceNo->setStyleSheet(styleSheet());
            status = true;
        }else{
            flashMsg = "This Invoice no doesnot need to be paid this much amount or it must be paid already";
            ui->lineEditInvoiceNo->setProperty("validationError",true);
            ui->lineEditInvoiceNo->setProperty("validationSuccess",false);
            ui->lineEditInvoiceNo->setStyleSheet(styleSheet());
            status = false;
        }
    }else{
        flashMsg = "Invoice Number is empty. Please give Invoice Number.";
        ui->lineEditInvoiceNo->setProperty("validationError",true);
        ui->lineEditInvoiceNo->setProperty("validationSuccess",false);
        ui->lineEditInvoiceNo->setStyleSheet(styleSheet());
        status = false;
    }
    ui->flashMsgUp->setText(flashMsg);
    return status;
}

bool TransactionForm::amountValid()
{
    bool status = false;
    QString flashMsg = "";
    ui->lineEditPaidAmount->installEventFilter(this);
    if(ui->lineEditPaidAmount->text().length() > 0){
        if(invoiceNoValid()){
            ui->lineEditPaidAmount->setProperty("validationError",false);
            ui->lineEditPaidAmount->setProperty("validationSuccess",true);
            ui->lineEditPaidAmount->setStyleSheet(styleSheet());
            status = true;
        }else{
            flashMsg = "This Invoice no doesnot need to be paid this much amount or it must be paid already.";
            ui->lineEditPaidAmount->setProperty("validationError",true);
            ui->lineEditPaidAmount->setProperty("validationSuccess",false);
            ui->lineEditPaidAmount->setStyleSheet(styleSheet());
            status = false;
        }
    }else{
        flashMsg = "Amount to be paid is empty. Please give the amount to be paid";
        ui->lineEditPaidAmount->setProperty("validationError",true);
        ui->lineEditPaidAmount->setProperty("validationSuccess",false);
        ui->lineEditPaidAmount->setStyleSheet(styleSheet());
        status = false;
    }
    ui->flashMsgUp->setText(flashMsg);
    return status;
}

bool TransactionForm::billStatus()
{
    int invoiceNo = ui->lineEditInvoiceNo->text().toInt();
    int balance, paid = 0, toBePaid, billId;
    QSqlQuery billQuery, transactionQuery;
    billQuery.prepare("Select * from bill where invoiceNo = :invoiceNo and status = 'A' and paidStatus = 'U'" );
    billQuery.bindValue(":invoiceNo", invoiceNo);
    billQuery.exec();
    if(billQuery.size() == 0)
        return false;
    else{
        while(billQuery.next()){
            balance = billQuery.value(7).toInt();
            billId = billQuery.value(0).toInt();
        }
        balance = -1*balance;
        transactionQuery.prepare("Select * from transactions where bill_id = :billId and status = 'A'");
        transactionQuery.bindValue(":billId",billId);
        transactionQuery.exec();
        if(transactionQuery.size() == 0){
            toBePaid = balance - ui->lineEditPaidAmount->text().toInt();
            if(toBePaid >= 0)
                return true;
        }else{
            while(transactionQuery.next())
                paid = paid+transactionQuery.value(3).toInt();
            toBePaid = balance-paid-ui->lineEditPaidAmount->text().toInt();
            if(toBePaid >= 0)
                return true;
        }
    }
    return false;
}