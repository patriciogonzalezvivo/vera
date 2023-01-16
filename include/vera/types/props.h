#pragma once

namespace vera {

enum HorizontalAlign {
    ALIGN_LEFT 	    = 1<<0,
    ALIGN_CENTER 	= 1<<1,
    ALIGN_RIGHT 	= 1<<2
};

enum VerticalAlign {
    ALIGN_TOP 	    = 1<<3,
    ALIGN_MIDDLE 	= 1<<4,
    ALIGN_BOTTOM 	= 1<<5,
    ALIGN_BASELINE  = 1<<6  // only for fonts
};

};