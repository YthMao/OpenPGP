/*
Partial.h
Partial Length Packet

Copyright (c) 2013 - 2017 Jason Lee @ calccrypto at gmail.com

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#ifndef __PARTIAL__
#define __PARTIAL__

#include "Packet.h"

namespace OpenPGP {
    namespace Packet {

        class Partial : public Tag {
            private:
                std::string stream;

                std::string show_title() const;

            public:
                typedef std::shared_ptr <Partial> Ptr;

                Partial();
                Partial(const std::string & data);
                void read(const std::string & data);
                std::string show(const std::size_t indents = 0, const std::size_t indent_size = 4) const;
                std::string raw() const;

                std::string get_stream() const;

                void set_stream(const std::string & data);

                Tag::Ptr clone() const;
        };
    }
}

#endif
