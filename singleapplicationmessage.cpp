// Copyright (c) Itay Grudev 2023
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// Permission is not granted to use this software or any of the associated files
// as sample data for the purposes of building machine learning models.
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#include "singleapplicationmessage.h"

#include <QIODevice>

SingleApplicationMessage::SingleApplicationMessage()
    : invalid( true )
{
}

SingleApplicationMessage::SingleApplicationMessage( MessageType type, quint16 instanceId, QByteArray content )
    : type( type ), instanceId( instanceId ), content( content ), invalid( false )
{}

SingleApplicationMessage::SingleApplicationMessage( QByteArray message )
    : invalid( false )
{
        QDataStream dataStream( &message, QIODevice::ReadOnly );
        qsizetype messageLength;
        quint16 messageChecksum;

        dataStream >> type;
        dataStream >> instanceId;
        dataStream >> messageLength;
        if( (qsizetype)(message.size() - sizeof(type) - sizeof(messageLength) - sizeof(quint16)) < messageLength ){
            invalid = true;
            return;
        }
        content = QByteArray( messageLength, Qt::Uninitialized );
        dataStream.readRawData( content.data(), messageLength );
        dataStream >> messageChecksum;

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        const quint16 computedChecksum = qChecksum( QByteArray(message.constData(), static_cast<quint32>( message.length() - sizeof(quint16) )));
#else
        const quint16 computedChecksum = qChecksum( message.constData(), static_cast<quint32>( message.length() - sizeof(quint16) ));
#endif

        if( messageChecksum != computedChecksum )
            invalid = true;
    }

SingleApplicationMessage:: operator QByteArray()
{
    QByteArray message;
    QDataStream dataStream( &message, QIODevice::WriteOnly );

#if (QT_VERSION >= QT_VERSION_CHECK(5, 6, 0))
    dataStream.setVersion(QDataStream::Qt_5_6);
#endif

    dataStream << static_cast<quint8>( type );
    dataStream << instanceId;
    dataStream << (qsizetype)content.size();
    dataStream << content;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    quint16 checksum = qChecksum( QByteArray( message.constData(), static_cast<quint32>( message.length() )));
#else
    quint16 checksum = qChecksum( message.constData(), static_cast<quint32>( messageMsg.length() ));
#endif
    dataStream << checksum;

    return message;
}
