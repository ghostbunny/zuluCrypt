/*
 * copyright: 2013
 * name : mhogo mchungu
 * email: mhogomchungu@gmail.com
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef LXQT_INTERNAL_WALLET_H
#define LXQT_INTERNAL_WALLET_H

#include "lxqt_wallet.h"
#include "../backend/lxqtwallet.h"
#include "password_dialog.h"
#include "task.h"
#include "changepassworddialog.h"

#include <QString>
#include <QByteArray>
#include <QDebug>
#include <QMessageBox>
#include "password_dialog.h"
#include <QPushButton>
#include <QDir>

class QWidget ;

namespace LxQt{

namespace Wallet{

class internalWallet : public LxQt::Wallet::Wallet
{
	Q_OBJECT
public:
	internalWallet() ;
	~internalWallet() ;
	bool addKey( const QString& key,const QByteArray& value ) ;
	void open( const QString& walletName,const QString& applicationName,const QString& password = QString() ) ;
	QByteArray readValue( const QString& key ) ;
	QVector<LxQt::Wallet::walletKeyValues> readAllKeyValues( void ) ;
	QStringList readAllKeys( void ) ;
	void deleteKey( const QString& key ) ;
	int walletSize( void )  ;
	void closeWallet( bool ) ;
	LxQt::Wallet::walletBackEnd backEnd( void ) ;
	bool walletIsOpened( void ) ;
	void setInterfaceObject( QWidget * parent ) ;
	QObject * qObject( void ) ;
	QString storagePath( void ) ;
	void changeWalletPassWord( const QString& walletName,const QString& applicationName = QString() ) ;
	QStringList managedWalletList( void ) ;
	QString localDefaultWalletName( void ) ;
	QString networkDefaultWalletName( void ) ;
	void setImage( const QString& ) ;
signals:
	void walletIsOpen( bool ) ;
	void passwordIsCorrect( bool ) ;
	void walletpassWordChanged( bool ) ;
	void getPassWord( QString ) ;
private slots:
	bool openWallet( QString ) ;
	void cancelled( void ) ;
	void taskResult( bool ) ;
	void taskResult_1( bool ) ;
	void password( QString,bool ) ;
private:
	bool openWallet( void ) ;
	lxqt_wallet_t m_wallet ;
	QString m_walletName ;
	QString m_applicationName ;
	QString m_password ;
	QString m_image ;
	QWidget * m_interfaceObject ;
};

}

}
#endif // LXQT_INTERNAL_WALLET_H
