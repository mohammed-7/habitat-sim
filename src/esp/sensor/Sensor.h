// Copyright (c) Facebook, Inc. and its affiliates.
// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.

#pragma once

#include <Corrade/Containers/Optional.h>

#include "esp/core/esp.h"

#include "esp/core/Buffer.h"
#include "esp/gfx/RenderCamera.h"
#include "esp/gfx/RenderTarget.h"
#include "esp/scene/SceneNode.h"

namespace esp {
namespace gfx {
class Simulator;
}

namespace sensor {

// Enumeration of types of sensors
enum class SensorType {
  NONE = 0,
  COLOR = 1,
  DEPTH = 2,
  NORMAL = 3,
  SEMANTIC = 4,
  PATH = 5,
  GOAL = 6,
  FORCE = 7,
  TENSOR = 8,
  TEXT = 9,
};

enum class ObservationSpaceType {
  NONE = 0,
  TENSOR = 1,
  TEXT = 2,
};

// Specifies the configuration parameters of a sensor
struct SensorSpec {
  std::string uuid = "rgba_camera";
  SensorType sensorType = SensorType::COLOR;
  std::string sensorSubtype = "pinhole";
  std::map<std::string, std::string> parameters = {{"near", "0.01"},
                                                   {"far", "1000"},
                                                   {"hfov", "90"}};
  vec3f position = {0, 1.5, 0};
  vec3f orientation = {0, 0, 0};
  vec2i resolution = {84, 84};
  int channels = 4;
  std::string encoding = "rgba_uint8";
  // description of Sensor observation space as gym.spaces.Dict()
  std::string observationSpace = "";
  bool gpu2gpuTransfer = false;
  ESP_SMART_POINTERS(SensorSpec)
};
bool operator==(const SensorSpec& a, const SensorSpec& b);
bool operator!=(const SensorSpec& a, const SensorSpec& b);

// Represents a particular sensor Observation
struct Observation {
  // TODO: populate this struct with raw data
  core::Buffer::ptr buffer;
  ESP_SMART_POINTERS(Observation)
};

struct ObservationSpace {
  ObservationSpaceType spaceType = ObservationSpaceType::TENSOR;
  core::DataType dataType = core::DataType::DT_UINT8;
  std::vector<size_t> shape;
  ESP_SMART_POINTERS(ObservationSpace)
};

// Represents a sensor that provides data from the environment to an agent
class Sensor : public Magnum::SceneGraph::AbstractFeature3D {
 public:
  explicit Sensor(scene::SceneNode& node, SensorSpec::ptr spec);
  virtual ~Sensor() { LOG(INFO) << "Deconstructing Sensor"; }

  // Get the scene node being attached to.
  scene::SceneNode& node() { return object(); }
  const scene::SceneNode& node() const { return object(); }

  // Overloads to avoid confusion
  scene::SceneNode& object() {
    return static_cast<scene::SceneNode&>(
        Magnum::SceneGraph::AbstractFeature3D::object());
  }
  const scene::SceneNode& object() const {
    return static_cast<const scene::SceneNode&>(
        Magnum::SceneGraph::AbstractFeature3D::object());
  }

  SensorSpec::ptr specification() const { return spec_; }

  //! Returns the size of the framebuffer corresponding to the sensor's
  //! resolution
  Magnum::Vector2i framebufferSize() const {
    // NB: The sensors resolution is in H x W format as that more cleanly
    // corresponds to the pratice of treating images as arrays that is used in
    // modern CV and DL However, graphics frameworks expected W x H format for
    // frame buffer sizes
    return {spec_->resolution[1], spec_->resolution[0]};
  }

  // can be called ONLY when it is attached to a scene node
  virtual void setTransformationFromSpec();

  virtual bool isVisualSensor() { return false; }

  // visual sensor should implement and override this function
  virtual void setProjectionMatrix(gfx::RenderCamera& targetCamera){};

  virtual bool getObservation(gfx::Simulator& sim, Observation& obs);
  virtual bool getObservationSpace(ObservationSpace& space);

  // The 2x2 matrix used to unproject depth values from those in the depth
  // buffer to meters
  virtual Corrade::Containers::Optional<Magnum::Vector2> depthUnprojection()
      const {
    return Corrade::Containers::NullOpt;
  };

  bool hasRenderTarget() const { return tgt_ != nullptr; }

  void bindRenderTarget(gfx::RenderTarget* tgt) {
    if (tgt->framebufferSize() != framebufferSize())
      throw std::runtime_error("RenderTarget is not the correct size");
    this->tgt_.reset(tgt);
  }

  gfx::RenderTarget& renderTarget() {
    if (!hasRenderTarget())
      throw std::runtime_error("Sensor has no rendering target");
    return *tgt_;
  }

 protected:
  SensorSpec::ptr spec_ = nullptr;
  core::Buffer::ptr buffer_ = nullptr;

  gfx::RenderTarget::uptr tgt_ = nullptr;

  ESP_SMART_POINTERS(Sensor)
};

// Represents a set of sensors, with each sensor being identified through a
// unique id

class SensorSuite {
 public:
  void add(Sensor::ptr sensor);
  void clear();
  ~SensorSuite() { LOG(INFO) << "Deconstructing SensorSuite"; }

  Sensor::ptr get(const std::string& uuid) const;
  std::map<std::string, Sensor::ptr>& getSensors() { return sensors_; }

 protected:
  std::map<std::string, Sensor::ptr> sensors_;

  ESP_SMART_POINTERS(SensorSuite)
};

}  // namespace sensor
}  // namespace esp
