/* Copyright 2014 yiyuanzhong@gmail.com (Yiyuan Zhong)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <gtest/gtest.h>
#include <signal.h>

#include "flinter/signals.h"

TEST(signalsTest, TestBlock)
{
    signals_block(SIGHUP);
}

TEST(signalsTest, TestBlocks)
{
    signals_block_some(SIGUSR1, SIGUSR2, SIGINT, SIGHUP, SIGQUIT, SIGTERM, 0);
    signals_unblock_some(SIGUSR1, SIGUSR2, SIGINT, SIGHUP, SIGQUIT, SIGTERM, 0);
}

TEST(signalsTest, TestAll)
{
    signals_default_all();
}
