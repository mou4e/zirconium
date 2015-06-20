// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

#include "base/bind.h"
#include "base/memory/ref_counted.h"
#include "base/strings/string_number_conversions.h"
#include "base/test/test_simple_task_runner.h"
#include "device/bluetooth/bluetooth_adapter.h"
#include "device/bluetooth/bluetooth_adapter_win.h"
#include "device/bluetooth/bluetooth_device.h"
#include "device/bluetooth/bluetooth_task_manager_win.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

const char kAdapterAddress[] = "A1:B2:C3:D4:E5:F6";
const char kAdapterName[] = "Bluetooth Adapter Name";

const char kTestAudioSdpName[] = "Audio";
const char kTestAudioSdpName2[] = "Audio2";
const char kTestAudioSdpBytes[] =
    "35510900000a00010001090001350319110a09000435103506190100090019350619001909"
    "010209000535031910020900093508350619110d090102090100250c417564696f20536f75"
    "726365090311090001";
const device::BluetoothUUID kTestAudioSdpUuid("110a");

void MakeDeviceState(const std::string& name,
                     const std::string& address,
                     device::BluetoothTaskManagerWin::DeviceState* state) {
  state->name = name;
  state->address = address;
  state->bluetooth_class = 0;
  state->authenticated = false;
  state->connected = false;
}

class AdapterObserver : public device::BluetoothAdapter::Observer {
 public:
  AdapterObserver() { ResetCounters(); }

  void ResetCounters() {
    num_present_changed_ = 0;
    num_powered_changed_ = 0;
    num_discovering_changed_ = 0;
    num_device_added_ = 0;
    num_device_removed_ = 0;
    num_device_changed_ = 0;
  }

  virtual void AdapterPresentChanged(
      device::BluetoothAdapter* adapter, bool present) override {
    num_present_changed_++;
  }

  virtual void AdapterPoweredChanged(
      device::BluetoothAdapter* adapter, bool powered) override {
    num_powered_changed_++;
  }

  virtual void AdapterDiscoveringChanged(
      device::BluetoothAdapter* adapter, bool discovering) override {
    num_discovering_changed_++;
  }

  virtual void DeviceAdded(
      device::BluetoothAdapter* adapter,
      device::BluetoothDevice* device) override {
    num_device_added_++;
  }

  virtual void DeviceRemoved(device::BluetoothAdapter* adapter,
                             device::BluetoothDevice* device) override {
    num_device_removed_++;
  }

  virtual void DeviceChanged(device::BluetoothAdapter* adapter,
                             device::BluetoothDevice* device) override {
    num_device_changed_++;
  }

  int num_present_changed() const { return num_present_changed_; }

  int num_powered_changed() const { return num_powered_changed_; }

  int num_discovering_changed() const { return num_discovering_changed_; }

  int num_device_added() const { return num_device_added_; }

  int num_device_removed() const { return num_device_removed_; }

  int num_device_changed() const { return num_device_changed_; }

 private:
  int num_present_changed_;
  int num_powered_changed_;
  int num_discovering_changed_;
  int num_device_added_;
  int num_device_removed_;
  int num_device_changed_;
};

}  // namespace

namespace device {

class BluetoothAdapterWinTest : public testing::Test {
 public:
  BluetoothAdapterWinTest()
      : ui_task_runner_(new base::TestSimpleTaskRunner()),
        bluetooth_task_runner_(new base::TestSimpleTaskRunner()),
        adapter_(new BluetoothAdapterWin(
          base::Bind(&BluetoothAdapterWinTest::RunInitCallback,
                     base::Unretained(this)))),
        adapter_win_(static_cast<BluetoothAdapterWin*>(adapter_.get())),
        init_callback_called_(false) {
    adapter_win_->InitForTest(ui_task_runner_, bluetooth_task_runner_);
  }

  virtual void SetUp() override {
    adapter_win_->AddObserver(&adapter_observer_);
    num_start_discovery_callbacks_ = 0;
    num_start_discovery_error_callbacks_ = 0;
    num_stop_discovery_callbacks_ = 0;
    num_stop_discovery_error_callbacks_ = 0;
  }

  virtual void TearDown() override {
    adapter_win_->RemoveObserver(&adapter_observer_);
  }

  void RunInitCallback() {
    init_callback_called_ = true;
  }

  void IncrementNumStartDiscoveryCallbacks() {
    num_start_discovery_callbacks_++;
  }

  void IncrementNumStartDiscoveryErrorCallbacks() {
    num_start_discovery_error_callbacks_++;
  }

  void IncrementNumStopDiscoveryCallbacks() {
    num_stop_discovery_callbacks_++;
  }

  void IncrementNumStopDiscoveryErrorCallbacks() {
    num_stop_discovery_error_callbacks_++;
  }

  void CallAddDiscoverySession(
      const base::Closure& callback,
      const BluetoothAdapter::ErrorCallback& error_callback) {
    adapter_win_->AddDiscoverySession(callback, error_callback);
  }

  void CallRemoveDiscoverySession(
      const base::Closure& callback,
      const BluetoothAdapter::ErrorCallback& error_callback) {
    adapter_win_->RemoveDiscoverySession(callback, error_callback);
  }

 protected:
  scoped_refptr<base::TestSimpleTaskRunner> ui_task_runner_;
  scoped_refptr<base::TestSimpleTaskRunner> bluetooth_task_runner_;
  scoped_refptr<BluetoothAdapter> adapter_;
  BluetoothAdapterWin* adapter_win_;
  AdapterObserver adapter_observer_;
  bool init_callback_called_;
  int num_start_discovery_callbacks_;
  int num_start_discovery_error_callbacks_;
  int num_stop_discovery_callbacks_;
  int num_stop_discovery_error_callbacks_;
};

TEST_F(BluetoothAdapterWinTest, AdapterNotPresent) {
  BluetoothTaskManagerWin::AdapterState state;
  adapter_win_->AdapterStateChanged(state);
  EXPECT_FALSE(adapter_win_->IsPresent());
}

TEST_F(BluetoothAdapterWinTest, AdapterPresent) {
  BluetoothTaskManagerWin::AdapterState state;
  state.address = kAdapterAddress;
  state.name = kAdapterName;
  adapter_win_->AdapterStateChanged(state);
  EXPECT_TRUE(adapter_win_->IsPresent());
}

TEST_F(BluetoothAdapterWinTest, AdapterPresentChanged) {
  BluetoothTaskManagerWin::AdapterState state;
  state.address = kAdapterAddress;
  state.name = kAdapterName;
  adapter_win_->AdapterStateChanged(state);
  EXPECT_EQ(1, adapter_observer_.num_present_changed());
  adapter_win_->AdapterStateChanged(state);
  EXPECT_EQ(1, adapter_observer_.num_present_changed());
  BluetoothTaskManagerWin::AdapterState empty_state;
  adapter_win_->AdapterStateChanged(empty_state);
  EXPECT_EQ(2, adapter_observer_.num_present_changed());
}

TEST_F(BluetoothAdapterWinTest, AdapterPoweredChanged) {
  BluetoothTaskManagerWin::AdapterState state;
  state.powered = true;
  adapter_win_->AdapterStateChanged(state);
  EXPECT_EQ(1, adapter_observer_.num_powered_changed());
  adapter_win_->AdapterStateChanged(state);
  EXPECT_EQ(1, adapter_observer_.num_powered_changed());
  state.powered = false;
  adapter_win_->AdapterStateChanged(state);
  EXPECT_EQ(2, adapter_observer_.num_powered_changed());
}

TEST_F(BluetoothAdapterWinTest, AdapterInitialized) {
  EXPECT_FALSE(adapter_win_->IsInitialized());
  EXPECT_FALSE(init_callback_called_);
  BluetoothTaskManagerWin::AdapterState state;
  adapter_win_->AdapterStateChanged(state);
  EXPECT_TRUE(adapter_win_->IsInitialized());
  EXPECT_TRUE(init_callback_called_);
}

TEST_F(BluetoothAdapterWinTest, SingleStartDiscovery) {
  bluetooth_task_runner_->ClearPendingTasks();
  CallAddDiscoverySession(
      base::Bind(&BluetoothAdapterWinTest::IncrementNumStartDiscoveryCallbacks,
                 base::Unretained(this)),
      BluetoothAdapter::ErrorCallback());
  EXPECT_TRUE(ui_task_runner_->GetPendingTasks().empty());
  EXPECT_EQ(1, bluetooth_task_runner_->GetPendingTasks().size());
  EXPECT_FALSE(adapter_->IsDiscovering());
  EXPECT_EQ(0, num_start_discovery_callbacks_);
  adapter_win_->DiscoveryStarted(true);
  ui_task_runner_->RunPendingTasks();
  EXPECT_TRUE(adapter_->IsDiscovering());
  EXPECT_EQ(1, num_start_discovery_callbacks_);
  EXPECT_EQ(1, adapter_observer_.num_discovering_changed());
}

TEST_F(BluetoothAdapterWinTest, SingleStartDiscoveryFailure) {
  CallAddDiscoverySession(
      base::Closure(),
      base::Bind(
          &BluetoothAdapterWinTest::IncrementNumStartDiscoveryErrorCallbacks,
          base::Unretained(this)));
  adapter_win_->DiscoveryStarted(false);
  ui_task_runner_->RunPendingTasks();
  EXPECT_FALSE(adapter_->IsDiscovering());
  EXPECT_EQ(1, num_start_discovery_error_callbacks_);
  EXPECT_EQ(0, adapter_observer_.num_discovering_changed());
}

TEST_F(BluetoothAdapterWinTest, MultipleStartDiscoveries) {
  bluetooth_task_runner_->ClearPendingTasks();
  int num_discoveries = 5;
  for (int i = 0; i < num_discoveries; i++) {
    CallAddDiscoverySession(
        base::Bind(
            &BluetoothAdapterWinTest::IncrementNumStartDiscoveryCallbacks,
            base::Unretained(this)),
        BluetoothAdapter::ErrorCallback());
    EXPECT_EQ(1, bluetooth_task_runner_->GetPendingTasks().size());
  }
  EXPECT_TRUE(ui_task_runner_->GetPendingTasks().empty());
  EXPECT_FALSE(adapter_->IsDiscovering());
  EXPECT_EQ(0, num_start_discovery_callbacks_);
  adapter_win_->DiscoveryStarted(true);
  ui_task_runner_->RunPendingTasks();
  EXPECT_TRUE(adapter_->IsDiscovering());
  EXPECT_EQ(num_discoveries, num_start_discovery_callbacks_);
  EXPECT_EQ(1, adapter_observer_.num_discovering_changed());
}

TEST_F(BluetoothAdapterWinTest, MultipleStartDiscoveriesFailure) {
  int num_discoveries = 5;
  for (int i = 0; i < num_discoveries; i++) {
    CallAddDiscoverySession(
        base::Closure(),
        base::Bind(
            &BluetoothAdapterWinTest::IncrementNumStartDiscoveryErrorCallbacks,
            base::Unretained(this)));
  }
  adapter_win_->DiscoveryStarted(false);
  ui_task_runner_->RunPendingTasks();
  EXPECT_FALSE(adapter_->IsDiscovering());
  EXPECT_EQ(num_discoveries, num_start_discovery_error_callbacks_);
  EXPECT_EQ(0, adapter_observer_.num_discovering_changed());
}

TEST_F(BluetoothAdapterWinTest, MultipleStartDiscoveriesAfterDiscovering) {
  CallAddDiscoverySession(
      base::Bind(&BluetoothAdapterWinTest::IncrementNumStartDiscoveryCallbacks,
                 base::Unretained(this)),
      BluetoothAdapter::ErrorCallback());
  adapter_win_->DiscoveryStarted(true);
  ui_task_runner_->RunPendingTasks();
  EXPECT_TRUE(adapter_->IsDiscovering());
  EXPECT_EQ(1, num_start_discovery_callbacks_);

  bluetooth_task_runner_->ClearPendingTasks();
  for (int i = 0; i < 5; i++) {
    int num_start_discovery_callbacks = num_start_discovery_callbacks_;
    CallAddDiscoverySession(
        base::Bind(
           &BluetoothAdapterWinTest::IncrementNumStartDiscoveryCallbacks,
           base::Unretained(this)),
        BluetoothAdapter::ErrorCallback());
    EXPECT_TRUE(adapter_->IsDiscovering());
    EXPECT_TRUE(bluetooth_task_runner_->GetPendingTasks().empty());
    EXPECT_TRUE(ui_task_runner_->GetPendingTasks().empty());
    EXPECT_EQ(num_start_discovery_callbacks + 1,
              num_start_discovery_callbacks_);
  }
  EXPECT_EQ(1, adapter_observer_.num_discovering_changed());
}

TEST_F(BluetoothAdapterWinTest, StartDiscoveryAfterDiscoveringFailure) {
  CallAddDiscoverySession(
      base::Closure(),
      base::Bind(
          &BluetoothAdapterWinTest::IncrementNumStartDiscoveryErrorCallbacks,
          base::Unretained(this)));
  adapter_win_->DiscoveryStarted(false);
  ui_task_runner_->RunPendingTasks();
  EXPECT_FALSE(adapter_->IsDiscovering());
  EXPECT_EQ(1, num_start_discovery_error_callbacks_);

  CallAddDiscoverySession(
      base::Bind(&BluetoothAdapterWinTest::IncrementNumStartDiscoveryCallbacks,
                 base::Unretained(this)),
      BluetoothAdapter::ErrorCallback());
  adapter_win_->DiscoveryStarted(true);
  ui_task_runner_->RunPendingTasks();
  EXPECT_TRUE(adapter_->IsDiscovering());
  EXPECT_EQ(1, num_start_discovery_callbacks_);
}

TEST_F(BluetoothAdapterWinTest, SingleStopDiscovery) {
  CallAddDiscoverySession(
      base::Closure(), BluetoothAdapter::ErrorCallback());
  adapter_win_->DiscoveryStarted(true);
  ui_task_runner_->ClearPendingTasks();
  CallRemoveDiscoverySession(
      base::Bind(&BluetoothAdapterWinTest::IncrementNumStopDiscoveryCallbacks,
                 base::Unretained(this)),
      BluetoothAdapter::ErrorCallback());
  EXPECT_TRUE(adapter_->IsDiscovering());
  EXPECT_EQ(0, num_stop_discovery_callbacks_);
  bluetooth_task_runner_->ClearPendingTasks();
  adapter_win_->DiscoveryStopped();
  ui_task_runner_->RunPendingTasks();
  EXPECT_FALSE(adapter_->IsDiscovering());
  EXPECT_EQ(1, num_stop_discovery_callbacks_);
  EXPECT_TRUE(bluetooth_task_runner_->GetPendingTasks().empty());
  EXPECT_EQ(2, adapter_observer_.num_discovering_changed());
}

TEST_F(BluetoothAdapterWinTest, MultipleStopDiscoveries) {
  int num_discoveries = 5;
  for (int i = 0; i < num_discoveries; i++) {
    CallAddDiscoverySession(
        base::Closure(), BluetoothAdapter::ErrorCallback());
  }
  adapter_win_->DiscoveryStarted(true);
  ui_task_runner_->ClearPendingTasks();
  bluetooth_task_runner_->ClearPendingTasks();
  for (int i = 0; i < num_discoveries - 1; i++) {
    CallRemoveDiscoverySession(
        base::Bind(&BluetoothAdapterWinTest::IncrementNumStopDiscoveryCallbacks,
                   base::Unretained(this)),
        BluetoothAdapter::ErrorCallback());
    EXPECT_TRUE(bluetooth_task_runner_->GetPendingTasks().empty());
    ui_task_runner_->RunPendingTasks();
    EXPECT_EQ(i + 1, num_stop_discovery_callbacks_);
  }
  CallRemoveDiscoverySession(
      base::Bind(&BluetoothAdapterWinTest::IncrementNumStopDiscoveryCallbacks,
                 base::Unretained(this)),
      BluetoothAdapter::ErrorCallback());
  EXPECT_EQ(1, bluetooth_task_runner_->GetPendingTasks().size());
  EXPECT_TRUE(adapter_->IsDiscovering());
  adapter_win_->DiscoveryStopped();
  ui_task_runner_->RunPendingTasks();
  EXPECT_FALSE(adapter_->IsDiscovering());
  EXPECT_EQ(num_discoveries, num_stop_discovery_callbacks_);
  EXPECT_EQ(2, adapter_observer_.num_discovering_changed());
}

TEST_F(BluetoothAdapterWinTest,
       StartDiscoveryAndStartDiscoveryAndStopDiscoveries) {
  CallAddDiscoverySession(
      base::Bind(&BluetoothAdapterWinTest::IncrementNumStartDiscoveryCallbacks,
                 base::Unretained(this)),
      BluetoothAdapter::ErrorCallback());
  adapter_win_->DiscoveryStarted(true);
  CallAddDiscoverySession(
      base::Bind(&BluetoothAdapterWinTest::IncrementNumStartDiscoveryCallbacks,
                 base::Unretained(this)),
      BluetoothAdapter::ErrorCallback());
  ui_task_runner_->ClearPendingTasks();
  bluetooth_task_runner_->ClearPendingTasks();
  CallRemoveDiscoverySession(
      base::Bind(&BluetoothAdapterWinTest::IncrementNumStopDiscoveryCallbacks,
                 base::Unretained(this)),
      BluetoothAdapter::ErrorCallback());
  EXPECT_TRUE(bluetooth_task_runner_->GetPendingTasks().empty());
  CallRemoveDiscoverySession(
      base::Bind(&BluetoothAdapterWinTest::IncrementNumStopDiscoveryCallbacks,
                 base::Unretained(this)),
      BluetoothAdapter::ErrorCallback());
  EXPECT_EQ(1, bluetooth_task_runner_->GetPendingTasks().size());
}

TEST_F(BluetoothAdapterWinTest,
       StartDiscoveryAndStopDiscoveryAndStartDiscovery) {
  CallAddDiscoverySession(
      base::Closure(), BluetoothAdapter::ErrorCallback());
  adapter_win_->DiscoveryStarted(true);
  EXPECT_TRUE(adapter_->IsDiscovering());
  CallRemoveDiscoverySession(
      base::Closure(), BluetoothAdapter::ErrorCallback());
  adapter_win_->DiscoveryStopped();
  EXPECT_FALSE(adapter_->IsDiscovering());
  CallAddDiscoverySession(
      base::Closure(), BluetoothAdapter::ErrorCallback());
  adapter_win_->DiscoveryStarted(true);
  EXPECT_TRUE(adapter_->IsDiscovering());
}

TEST_F(BluetoothAdapterWinTest, StartDiscoveryBeforeDiscoveryStopped) {
  CallAddDiscoverySession(
      base::Closure(), BluetoothAdapter::ErrorCallback());
  adapter_win_->DiscoveryStarted(true);
  CallRemoveDiscoverySession(
      base::Closure(), BluetoothAdapter::ErrorCallback());
  CallAddDiscoverySession(
      base::Closure(), BluetoothAdapter::ErrorCallback());
  bluetooth_task_runner_->ClearPendingTasks();
  adapter_win_->DiscoveryStopped();
  EXPECT_EQ(1, bluetooth_task_runner_->GetPendingTasks().size());
}

TEST_F(BluetoothAdapterWinTest, StopDiscoveryWithoutStartDiscovery) {
  CallRemoveDiscoverySession(
      base::Closure(),
      base::Bind(
          &BluetoothAdapterWinTest::IncrementNumStopDiscoveryErrorCallbacks,
          base::Unretained(this)));
  EXPECT_EQ(1, num_stop_discovery_error_callbacks_);
}

TEST_F(BluetoothAdapterWinTest, StopDiscoveryBeforeDiscoveryStarted) {
  CallAddDiscoverySession(
      base::Closure(), BluetoothAdapter::ErrorCallback());
  CallRemoveDiscoverySession(
      base::Closure(), BluetoothAdapter::ErrorCallback());
  bluetooth_task_runner_->ClearPendingTasks();
  adapter_win_->DiscoveryStarted(true);
  EXPECT_EQ(1, bluetooth_task_runner_->GetPendingTasks().size());
}

TEST_F(BluetoothAdapterWinTest, StartAndStopBeforeDiscoveryStarted) {
  int num_expected_start_discoveries = 3;
  int num_expected_stop_discoveries = 2;
  for (int i = 0; i < num_expected_start_discoveries; i++) {
    CallAddDiscoverySession(
        base::Bind(
            &BluetoothAdapterWinTest::IncrementNumStartDiscoveryCallbacks,
            base::Unretained(this)),
        BluetoothAdapter::ErrorCallback());
  }
  for (int i = 0; i < num_expected_stop_discoveries; i++) {
    CallRemoveDiscoverySession(
        base::Bind(
            &BluetoothAdapterWinTest::IncrementNumStopDiscoveryCallbacks,
            base::Unretained(this)),
        BluetoothAdapter::ErrorCallback());
  }
  bluetooth_task_runner_->ClearPendingTasks();
  adapter_win_->DiscoveryStarted(true);
  EXPECT_TRUE(bluetooth_task_runner_->GetPendingTasks().empty());
  ui_task_runner_->RunPendingTasks();
  EXPECT_EQ(num_expected_start_discoveries, num_start_discovery_callbacks_);
  EXPECT_EQ(num_expected_stop_discoveries, num_stop_discovery_callbacks_);
}

TEST_F(BluetoothAdapterWinTest, StopDiscoveryBeforeDiscoveryStartedAndFailed) {
  CallAddDiscoverySession(
      base::Closure(),
      base::Bind(
          &BluetoothAdapterWinTest::IncrementNumStartDiscoveryErrorCallbacks,
          base::Unretained(this)));
  CallRemoveDiscoverySession(
      base::Bind(
          &BluetoothAdapterWinTest::IncrementNumStopDiscoveryCallbacks,
          base::Unretained(this)),
      BluetoothAdapter::ErrorCallback());
  ui_task_runner_->ClearPendingTasks();
  adapter_win_->DiscoveryStarted(false);
  ui_task_runner_->RunPendingTasks();
  EXPECT_EQ(1, num_start_discovery_error_callbacks_);
  EXPECT_EQ(1, num_stop_discovery_callbacks_);
  EXPECT_EQ(0, adapter_observer_.num_discovering_changed());
}

TEST_F(BluetoothAdapterWinTest, DevicesPolled) {
  BluetoothTaskManagerWin::DeviceState* android_phone_state =
      new BluetoothTaskManagerWin::DeviceState();
  MakeDeviceState("phone", "A1:B2:C3:D4:E5:E0", android_phone_state);
  BluetoothTaskManagerWin::DeviceState* laptop_state =
      new BluetoothTaskManagerWin::DeviceState();
  MakeDeviceState("laptop", "A1:B2:C3:D4:E5:E1", laptop_state);
  BluetoothTaskManagerWin::DeviceState* iphone_state =
      new BluetoothTaskManagerWin::DeviceState();
  MakeDeviceState("phone", "A1:B2:C3:D4:E5:E2", iphone_state);
  ScopedVector<BluetoothTaskManagerWin::DeviceState> devices;
  devices.push_back(android_phone_state);
  devices.push_back(laptop_state);
  devices.push_back(iphone_state);

  // Add 3 devices
  adapter_observer_.ResetCounters();
  adapter_win_->DevicesPolled(devices);
  EXPECT_EQ(3, adapter_observer_.num_device_added());
  EXPECT_EQ(0, adapter_observer_.num_device_removed());
  EXPECT_EQ(0, adapter_observer_.num_device_changed());

  // Change a device name
  android_phone_state->name = "phone2";
  adapter_observer_.ResetCounters();
  adapter_win_->DevicesPolled(devices);
  EXPECT_EQ(0, adapter_observer_.num_device_added());
  EXPECT_EQ(0, adapter_observer_.num_device_removed());
  EXPECT_EQ(1, adapter_observer_.num_device_changed());

  // Change a device address
  android_phone_state->address = "A1:B2:C3:D4:E5:E6";
  adapter_observer_.ResetCounters();
  adapter_win_->DevicesPolled(devices);
  EXPECT_EQ(1, adapter_observer_.num_device_added());
  EXPECT_EQ(1, adapter_observer_.num_device_removed());
  EXPECT_EQ(0, adapter_observer_.num_device_changed());

  // Remove a device
  devices.erase(devices.begin());
  adapter_observer_.ResetCounters();
  adapter_win_->DevicesPolled(devices);
  EXPECT_EQ(0, adapter_observer_.num_device_added());
  EXPECT_EQ(1, adapter_observer_.num_device_removed());
  EXPECT_EQ(0, adapter_observer_.num_device_changed());

  // Add a service
  BluetoothTaskManagerWin::ServiceRecordState* audio_state =
      new BluetoothTaskManagerWin::ServiceRecordState();
  audio_state->name = kTestAudioSdpName;
  base::HexStringToBytes(kTestAudioSdpBytes, &audio_state->sdp_bytes);
  laptop_state->service_record_states.push_back(audio_state);
  adapter_observer_.ResetCounters();
  adapter_win_->DevicesPolled(devices);
  EXPECT_EQ(0, adapter_observer_.num_device_added());
  EXPECT_EQ(0, adapter_observer_.num_device_removed());
  EXPECT_EQ(1, adapter_observer_.num_device_changed());

  // Change a service
  audio_state->name = kTestAudioSdpName2;
  adapter_observer_.ResetCounters();
  adapter_win_->DevicesPolled(devices);
  EXPECT_EQ(0, adapter_observer_.num_device_added());
  EXPECT_EQ(0, adapter_observer_.num_device_removed());
  EXPECT_EQ(1, adapter_observer_.num_device_changed());

  // Remove a service
  laptop_state->service_record_states.clear();
  adapter_observer_.ResetCounters();
  adapter_win_->DevicesPolled(devices);
  EXPECT_EQ(0, adapter_observer_.num_device_added());
  EXPECT_EQ(0, adapter_observer_.num_device_removed());
  EXPECT_EQ(1, adapter_observer_.num_device_changed());
}

}  // namespace device