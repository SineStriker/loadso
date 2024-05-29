#ifndef INTERFACE_H
#define INTERFACE_H

namespace LoadSO {

    class Interface {
    public:
        virtual ~Interface() = default;

        virtual const char *key() const = 0;
    };

}

#endif // INTERFACE_H
