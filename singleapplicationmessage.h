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

#ifndef SINGLEAPPLICATIONMESSAGE_H
#define SINGLEAPPLICATIONMESSAGE_H

#include <QByteArray>

struct SingleApplicationMessage {
    enum MessageType : quint8 {
        Acknowledge,
        NewInstance,
        Message,
    };

    MessageType type;
    quint16 instanceId;
    QByteArray content;
    bool invalid;

    /**
     * @brief Create an unitialized SingleApplicationMessage
     * The message is initialized with invalid = true
     */
    SingleApplicationMessage();

    /**
     * @brief Create a SingleApplicationMessage
     * @param type
     * @param instanceId
     * @param content
     */
    SingleApplicationMessage( MessageType type, quint16 instanceId, QByteArray content );

    /**
     * @brief Create a SingleApplicationMessage from a QByteArray containing a message
     * @param message
     */
    SingleApplicationMessage( QByteArray message );

    /**
     * @brief Convert a SingleApplicationMessage to a QByteArray
     * @return QByteArray
     */
    operator QByteArray();
};


#endif // SINGLEAPPLICATIONMESSAGE_H
