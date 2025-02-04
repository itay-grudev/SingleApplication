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

#ifndef MESSAGE_CODER_H
#define MESSAGE_CODER_H

#include <QByteArray>
#include <QException>

#include "singleapplication.h"

class MessageCoder : public QObject {
Q_OBJECT
public:
    /**
     * @brief Constructs MessageCoder from a QLocalSocket
     * @param message
     */
    MessageCoder( QLocalSocket *socket );

    /**
     * @brief Send a MessageCoder on a QDataStream
     * @param type
     * @param instanceId
     * @param content
     */
    bool sendMessage( SingleApplication::MessageType type, quint16 instanceId, QByteArray content );

Q_SIGNALS:
    void messageReceived( SingleApplication::Message message );

private Q_SLOTS:
    void slotDataAvailable();


private:
    QLocalSocket *socket;
    QDataStream dataStream;
};


#endif // MESSAGE_CODER_H
