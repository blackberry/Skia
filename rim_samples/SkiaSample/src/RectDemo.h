/*
 * Copyright (c) 2012 Research In Motion Limited.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef RECTDEMO_H_
#define RECTDEMO_H_

#include "DemoBase.h"

class RectDemo: public DemoBase {
public:
    RectDemo(SkCanvas *c, int w, int h);
    ~RectDemo() {}

    virtual void draw();
};

#endif /* RECTDEMO_H_ */
