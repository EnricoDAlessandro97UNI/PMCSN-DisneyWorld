#include "block4_helper.h"
#include "../finite_helper.h"

float get_failure_probability() {
    SelectStream(5);
    return (float)(Uniform(0.0, 1.0));
}
