#include <iostream>
#include "offline/index_builder.h"

int main() {
    search::IndexBuilder builder("/home/ubuntu/project/data/origin/memnews_5w_without_tag");
    builder.SegmentRawData();
    return 0;
}
