#pragma once

class ISense  {
    public:
        virtual int read();         // Returns current reading, or average if there's a cyclic buffer
        virtual void update();      // If there's cyclic avereging buffer, this will maintaint it
};