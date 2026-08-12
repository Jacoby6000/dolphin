// Copyright 2026 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <optional>
#include <span>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "Common/CommonTypes.h"
#include "Core/Core.h"
#include "Core/Debugger/DAP/DapMemoryEngine.h"
#include "Core/HW/AddressSpace.h"
#include "Core/HW/Memmap.h"
#include "Core/PowerPC/PowerPC.h"
#include "Core/System.h"

namespace
{
constexpr u32 DATA_ADDRESS = 0x00004000;
constexpr u32 SCAN_ADDRESS = 0x80004000;

class DapMemoryEngineTest : public ::testing::Test
{
protected:
  void SetUp() override
  {
    auto& system = Core::System::GetInstance();
    system.GetMemory().Init();
    AddressSpace::Init();

    Core::DeclareAsCPUThread();
    auto& power_pc = system.GetPowerPC();
    power_pc.Reset();
    auto& ppc_state = system.GetPPCState();
    ppc_state.msr.IR = 0;
    ppc_state.msr.DR = 0;
    power_pc.MSRUpdated();
    Core::UndeclareAsCPUThread();

    m_engine = std::make_unique<DAP::DapMemoryEngine>(
        system, [this](DAP::MemoryScanTerminalEvent event) {
          std::lock_guard lock(m_event_mutex);
          m_events.emplace_back(std::move(event));
          m_event_cv.notify_all();
        });
  }

  void TearDown() override
  {
    m_engine.reset();
    AddressSpace::Shutdown();
    Core::System::GetInstance().GetMemory().Shutdown();
  }

  void WriteBytes(u32 address, std::span<const u8> bytes)
  {
    Core::System::GetInstance().GetMemory().CopyToEmu(address, bytes.data(), bytes.size());
  }

  std::optional<DAP::MemoryScanTerminalEvent> WaitForEvent(size_t index)
  {
    std::unique_lock lock(m_event_mutex);
    if (!m_event_cv.wait_for(lock, std::chrono::seconds(5),
                             [&] { return m_events.size() > index; }))
    {
      return std::nullopt;
    }
    return m_events[index];
  }

  DAP::MemoryScanStartConfig MakeConfig(DAP::MemoryScanDataType data_type, u32 start, u32 size,
                                        std::string value)
  {
    DAP::MemoryScanStartConfig config;
    config.ranges.push_back({start, start + size});
    config.data_type = data_type;
    config.filter = DAP::MemoryScanFilter::Exact;
    config.value = std::move(value);
    config.pause_during_scan = true;
    return config;
  }

  std::unique_ptr<DAP::DapMemoryEngine> m_engine;
  std::mutex m_event_mutex;
  std::condition_variable m_event_cv;
  std::vector<DAP::MemoryScanTerminalEvent> m_events;
};

TEST_F(DapMemoryEngineTest, ExactScanSupportsEveryNumericType)
{
  struct Case
  {
    DAP::MemoryScanDataType data_type;
    std::vector<u8> bytes;
    std::string value;
  };
  const std::vector<Case> cases{
      {DAP::MemoryScanDataType::U8, {0x7f}, "127"},
      {DAP::MemoryScanDataType::U16, {0x12, 0x34}, "4660"},
      {DAP::MemoryScanDataType::U32, {0xde, 0xad, 0xbe, 0xef}, "3735928559"},
      {DAP::MemoryScanDataType::U64,
       {0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01},
       "2305843009213693953"},
      {DAP::MemoryScanDataType::S8, {0xff}, "-1"},
      {DAP::MemoryScanDataType::S16, {0xff, 0xfe}, "-2"},
      {DAP::MemoryScanDataType::S32, {0xff, 0xff, 0xff, 0xfd}, "-3"},
      {DAP::MemoryScanDataType::S64,
       {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfc},
       "-4"},
      {DAP::MemoryScanDataType::F32, {0x3f, 0xc0, 0x00, 0x00}, "1.5"},
      {DAP::MemoryScanDataType::F64,
       {0xc0, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
       "-2.25"},
  };

  for (size_t i = 0; i < cases.size(); ++i)
  {
    const Case& test = cases[i];
    WriteBytes(DATA_ADDRESS, test.bytes);
    const auto accepted =
        m_engine->StartScan(MakeConfig(test.data_type, SCAN_ADDRESS,
                                      static_cast<u32>(test.bytes.size()), test.value));
    ASSERT_TRUE(accepted.has_value());

    const auto terminal = WaitForEvent(i);
    ASSERT_TRUE(terminal.has_value());
    EXPECT_EQ(terminal->event, "dolphin_memoryScanCompleted");
    EXPECT_EQ(terminal->result_count, 1u);

    const auto page = m_engine->GetResults(accepted->scan_id, 0, 1);
    ASSERT_TRUE(page.has_value());
    ASSERT_EQ(page->results.size(), 1u);
    EXPECT_EQ(page->results[0].address, SCAN_ADDRESS);
    EXPECT_EQ(page->results[0].raw, test.bytes);
    EXPECT_TRUE(m_engine->Dispose(accepted->scan_id));
  }
}

TEST_F(DapMemoryEngineTest, U64IncreasedByUsesExactIntegerArithmetic)
{
  const std::vector<u8> initial{0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  WriteBytes(DATA_ADDRESS, initial);
  auto config = MakeConfig(DAP::MemoryScanDataType::U64, SCAN_ADDRESS, 8, "0");
  config.filter = DAP::MemoryScanFilter::Unknown;
  config.value.reset();
  const auto accepted = m_engine->StartScan(config);
  ASSERT_TRUE(accepted.has_value());
  ASSERT_TRUE(WaitForEvent(0).has_value());

  const std::vector<u8> changed{0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01};
  WriteBytes(DATA_ADDRESS, changed);
  DAP::MemoryScanRefineConfig refine;
  refine.scan_id = accepted->scan_id;
  refine.filter = DAP::MemoryScanFilter::IncreasedBy;
  refine.value = "1";
  const auto refined = m_engine->RefineScan(refine);
  ASSERT_TRUE(refined.has_value());

  const auto terminal = WaitForEvent(1);
  ASSERT_TRUE(terminal.has_value());
  EXPECT_EQ(terminal->event, "dolphin_memoryScanCompleted");
  EXPECT_EQ(terminal->generation, 2u);
  EXPECT_EQ(terminal->result_count, 1u);
}

TEST_F(DapMemoryEngineTest, CancelledRefinementPreservesCommittedGeneration)
{
  const std::vector<u8> bytes{0xde, 0xad, 0xbe, 0xef};
  WriteBytes(DATA_ADDRESS, bytes);
  auto config = MakeConfig(DAP::MemoryScanDataType::U32, SCAN_ADDRESS, 4, "0");
  config.filter = DAP::MemoryScanFilter::Unknown;
  config.value.reset();
  const auto accepted = m_engine->StartScan(config);
  ASSERT_TRUE(accepted.has_value());
  ASSERT_TRUE(WaitForEvent(0).has_value());

  auto core_scan_lock = DAP::DapMemoryEngine::TryLockCoreScan();
  ASSERT_TRUE(core_scan_lock.owns_lock());
  DAP::MemoryScanRefineConfig refine;
  refine.scan_id = accepted->scan_id;
  refine.filter = DAP::MemoryScanFilter::Changed;
  const auto refined = m_engine->RefineScan(refine);
  ASSERT_TRUE(refined.has_value());
  ASSERT_TRUE(m_engine->Cancel(accepted->scan_id));
  core_scan_lock.unlock();

  const auto terminal = WaitForEvent(1);
  ASSERT_TRUE(terminal.has_value());
  EXPECT_EQ(terminal->event, "dolphin_memoryScanCancelled");
  EXPECT_EQ(terminal->generation, 1u);
  EXPECT_EQ(terminal->result_count, 1u);
  const auto status = m_engine->GetStatus(accepted->scan_id);
  ASSERT_TRUE(status.has_value());
  EXPECT_EQ(status->state, "cancelled");
  EXPECT_EQ(status->generation, 1u);
  EXPECT_EQ(status->result_count, 1u);
}

TEST_F(DapMemoryEngineTest, DisposeCancelsJobWaitingForCore)
{
  const std::vector<u8> bytes{0x01};
  WriteBytes(DATA_ADDRESS, bytes);
  auto core_scan_lock = DAP::DapMemoryEngine::TryLockCoreScan();
  ASSERT_TRUE(core_scan_lock.owns_lock());
  const auto accepted =
      m_engine->StartScan(MakeConfig(DAP::MemoryScanDataType::U8, SCAN_ADDRESS, 1, "1"));
  ASSERT_TRUE(accepted.has_value());
  ASSERT_TRUE(m_engine->Dispose(accepted->scan_id));
  core_scan_lock.unlock();

  const auto terminal = WaitForEvent(0);
  ASSERT_TRUE(terminal.has_value());
  EXPECT_EQ(terminal->event, "dolphin_memoryScanCancelled");
  EXPECT_FALSE(m_engine->GetStatus(accepted->scan_id).has_value());
}

TEST_F(DapMemoryEngineTest, AlignmentAndResultPagingUseAbsoluteAddresses)
{
  const std::vector<u8> bytes{0xff, 0xff, 0xff, 0x00, 0x00, 0x00,
                              0x05, 0x00, 0x00, 0x00, 0x05};
  WriteBytes(DATA_ADDRESS + 1, bytes);
  auto config = MakeConfig(DAP::MemoryScanDataType::U32, SCAN_ADDRESS + 1,
                           static_cast<u32>(bytes.size()), "5");
  config.aligned = true;
  const auto accepted = m_engine->StartScan(config);
  ASSERT_TRUE(accepted.has_value());
  const auto terminal = WaitForEvent(0);
  ASSERT_TRUE(terminal.has_value());
  ASSERT_EQ(terminal->result_count, 2u);

  const auto page = m_engine->GetResults(accepted->scan_id, 1, 1);
  ASSERT_TRUE(page.has_value());
  ASSERT_EQ(page->results.size(), 1u);
  EXPECT_EQ(page->results[0].address, SCAN_ADDRESS + 8);
}

TEST_F(DapMemoryEngineTest, RemoveResultsAndUndoPreserveImmutableGenerations)
{
  const std::vector<u8> bytes{1, 2, 3, 4};
  WriteBytes(DATA_ADDRESS, bytes);
  auto config = MakeConfig(DAP::MemoryScanDataType::U8, SCAN_ADDRESS, 4, "0");
  config.filter = DAP::MemoryScanFilter::Unknown;
  config.value.reset();
  const auto accepted = m_engine->StartScan(config);
  ASSERT_TRUE(accepted.has_value());
  ASSERT_TRUE(WaitForEvent(0).has_value());

  const auto removed =
      m_engine->RemoveResults(accepted->scan_id, {SCAN_ADDRESS + 1, SCAN_ADDRESS + 1,
                                                  SCAN_ADDRESS + 20});
  ASSERT_TRUE(removed.has_value());
  EXPECT_EQ(removed->generation, 2u);
  EXPECT_EQ(removed->result_count, 3u);
  EXPECT_EQ(removed->removed_count, 1u);
  EXPECT_TRUE(removed->can_undo);

  const auto no_change = m_engine->RemoveResults(accepted->scan_id, {SCAN_ADDRESS + 1});
  ASSERT_TRUE(no_change.has_value());
  EXPECT_EQ(no_change->generation, 2u);
  EXPECT_EQ(no_change->removed_count, 0u);

  const auto undone = m_engine->Undo(accepted->scan_id);
  ASSERT_TRUE(undone.has_value());
  EXPECT_EQ(undone->generation, 1u);
  EXPECT_EQ(undone->result_count, 4u);
  EXPECT_FALSE(undone->can_undo);
  EXPECT_FALSE(m_engine->Undo(accepted->scan_id).has_value());

  const auto removed_again = m_engine->RemoveResults(accepted->scan_id, {SCAN_ADDRESS + 2});
  ASSERT_TRUE(removed_again.has_value());
  EXPECT_EQ(removed_again->generation, 3u);
  EXPECT_EQ(removed_again->result_count, 3u);
}
}  // namespace
