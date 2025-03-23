#include "ecs.h"

// ----------------------------------------------------------------------
//  Archetype
// ----------------------------------------------------------------------

Archetype::Archetype(ComponentSignature signature) : signature(signature) {
  for (ComponentTypeID type : signature) {
    data[type] = std::vector<uint8_t>();
  }
}

// ----------------------------------------------------------------------
// Utilities
// ----------------------------------------------------------------------

// Utility to generate unique component type IDs at runtime (one per component
// type)
ComponentTypeID generateComponentTypeID() {
  static ComponentTypeID lastID = 0;
  return lastID++;
}

// ----------------------------------------------------------------------
// ECS
// ----------------------------------------------------------------------

Archetype *ECS::findOrCreateArchetype(const ComponentSignature &signature) {
  // Check if an archetype with this signature already exists
  for (Archetype &arch : archetypes) {
    if (arch.signature == signature) {
      return &arch;
    }
  }
  // If not, create a new one
  archetypes.emplace_back(Archetype(signature));
  return &archetypes.back();
}

void ECS::storeComponentData(Archetype *arch, ComponentTypeID type,
                             const void *compData, size_t compSize) {
  std::vector<uint8_t> &buffer = arch->data[type];
  // Resize the byte buffer to fit one more component of this type
  size_t offset = buffer.size();
  buffer.resize(offset + compSize);
  // Copy component bytes into the buffer
  std::memcpy(buffer.data() + offset, compData, compSize);
}

void ECS::copyComponentData(Archetype *srcArch, Archetype *dstArch,
                            ComponentTypeID type, size_t srcIndex) {
  std::vector<uint8_t> &srcBuffer = srcArch->data[type];
  std::vector<uint8_t> &dstBuffer = dstArch->data[type];

  // buffer size in bytes / number of components = component size in bytes
  size_t componentSize = srcBuffer.size() / srcArch->entities.size();

  // Ensure destination has space for the new component
  size_t dstOffset = dstBuffer.size();
  dstBuffer.resize(dstOffset + componentSize);

  // Copy the component bytes from source index to end of destination buffer
  const uint8_t *srcPtr = srcBuffer.data() + srcIndex * componentSize;
  std::memcpy(dstBuffer.data() + dstOffset, srcPtr, componentSize);
}

void ECS::removeEntityFromArchetype(Archetype *arch, size_t index,
                                    Entity entity) {
  // Remove entity at 'index' from arch->entities, maintaining dense arrays
  size_t lastIndex = arch->entities.size() - 1;
  Entity lastEntity = arch->entities[lastIndex];
  // Swap the last entity into the removed spot
  arch->entities[index] = lastEntity;

  // Update each component array: move last element's bytes into removed
  // element's slot
  for (ComponentTypeID type : arch->signature) {
    std::vector<uint8_t> &buffer = arch->data[type];
    size_t componentSize = buffer.size() / arch->entities.size();
    uint8_t *removedPtr = buffer.data() + index * componentSize;
    uint8_t *lastPtr = buffer.data() + lastIndex * componentSize;
    // Copy last element's bytes over the removed element's bytes
    std::memcpy(removedPtr, lastPtr, componentSize);

    // Shrink buffer
    buffer.resize(buffer.size() - componentSize);
  }
  arch->entities.pop_back();

  // Update the moved entity's record to new index in this archetype
  entityLocations[lastEntity] = {arch, index};
}
