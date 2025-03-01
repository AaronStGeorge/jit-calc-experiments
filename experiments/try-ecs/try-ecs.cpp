#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <optional>
#include <tuple>
#include <unordered_map>
#include <vector>

using Entity = uint32_t;
using ComponentTypeID = uint32_t;

// Utility to generate unique component type IDs at runtime (one per component
// type)
inline ComponentTypeID generateComponentTypeID() {
  static ComponentTypeID lastID = 0;
  return lastID++;
}

template <typename Component> ComponentTypeID componentTypeID() {
  static ComponentTypeID typeID = generateComponentTypeID();
  return typeID;
}

struct Archetype {
  std::vector<ComponentTypeID>
      signature;                // Sorted component types in this archetype
  std::vector<Entity> entities; // Entities in this archetype
  std::unordered_map<ComponentTypeID, std::vector<uint8_t>>
      data; // Raw data arrays

  // Constructor initializes the component arrays for given types
  Archetype(const std::vector<ComponentTypeID> &types) : signature(types) {
    for (ComponentTypeID type : types) {
      data[type] = std::vector<uint8_t>(); // empty array, will reserve on use
    }
  }
};

struct EntityRecord {
  Archetype *archetype;
  size_t index; // index of the entity within the archetype's arrays
};

template <typename... Components> class Query;

class ECS {
public:
  // Create a new entity with a given set of components
  template <typename... Components> Entity createEntity(Components &&...comps) {
    // Build and sort the signature (component type IDs) for this set of
    // components
    std::vector<ComponentTypeID> signature = {componentTypeID<Components>()...};
    std::sort(signature.begin(), signature.end());

    // Find or create an archetype for this signature
    Archetype *arch = findOrCreateArchetype(signature);

    // Create a new entity ID
    Entity entity = nextEntity++;
    // Add entity to archetype
    size_t index = arch->entities.size();
    arch->entities.push_back(entity);

    // Store each component's data in the archetype's arrays
    // Using fold expression to handle the parameter pack
    (storeComponentData(arch, componentTypeID<Components>(), &comps,
                        sizeof(Components)),
     ...);

    // Record the entity's location
    entityLocations[entity] = {arch, index};
    return entity;
  }

  // Add a component to an existing entity (moving it to a new archetype)
  template <typename Component>
  void addComponent(Entity entity, Component &&comp) {
    // Get current archetype and index of the entity
    EntityRecord rec = entityLocations[entity];
    Archetype *oldArch = rec.archetype;
    size_t oldIndex = rec.index;
    // Build new signature = old signature + new component type
    std::vector<ComponentTypeID> newSig = oldArch->signature;
    ComponentTypeID newType = componentTypeID<Component>();
    // If component already exists, do nothing
    if (std::find(newSig.begin(), newSig.end(), newType) != newSig.end()) {
      return;
    }
    newSig.push_back(newType);
    std::sort(newSig.begin(), newSig.end());

    // Find/create the new archetype
    Archetype *newArch = findOrCreateArchetype(newSig);
    // Append entity to new archetype
    size_t newIndex = newArch->entities.size();
    newArch->entities.push_back(entity);
    // Copy over this entity's components from oldArch to newArch
    for (ComponentTypeID type : oldArch->signature) {
      // Copy component data from old archetype
      copyComponentData(oldArch, newArch, type, oldIndex);
    }
    // Add the new component's data
    storeComponentData(newArch, newType, &comp, sizeof(Component));
    // Update entity location to new archetype
    entityLocations[entity] = {newArch, newIndex};
    // Remove entity from old archetype (swap-remove to keep array dense)
    removeEntityFromArchetype(oldArch, oldIndex, entity);
  }

  template <typename... Components> Query<Components...> query() {
    return Query<Components...>(*this);
  }

private:
  Entity nextEntity = 1;
  std::vector<Archetype> archetypes;
  std::unordered_map<Entity, EntityRecord> entityLocations;

  Archetype *
  findOrCreateArchetype(const std::vector<ComponentTypeID> &signature) {
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

  void storeComponentData(Archetype *arch, ComponentTypeID type,
                          const void *compData, size_t compSize) {
    std::vector<uint8_t> &buffer = arch->data[type];
    // Resize the byte buffer to fit one more component of this type
    size_t offset = buffer.size();
    buffer.resize(offset + compSize);
    // Copy component bytes into the buffer
    std::memcpy(buffer.data() + offset, compData, compSize);
  }

  void copyComponentData(Archetype *srcArch, Archetype *dstArch,
                         ComponentTypeID type, size_t srcIndex) {
    // Get source data buffer and destination buffer for the component type
    std::vector<uint8_t> &srcBuffer = srcArch->data[type];
    std::vector<uint8_t> &dstBuffer = dstArch->data[type];
    size_t compSize = srcBuffer.size() / srcArch->entities.size();
    // Ensure destination has space for the new component
    size_t dstOffset = dstBuffer.size();
    dstBuffer.resize(dstOffset + compSize);
    // Copy the component bytes from source index to end of destination buffer
    const uint8_t *srcPtr = srcBuffer.data() + srcIndex * compSize;
    std::memcpy(dstBuffer.data() + dstOffset, srcPtr, compSize);
  }

  void removeEntityFromArchetype(Archetype *arch, size_t index, Entity entity) {
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
      // Copy last element's bytes over the removed element's bytes
      uint8_t *removedPtr = buffer.data() + index * componentSize;
      uint8_t *lastPtr = buffer.data() + lastIndex * componentSize;
      std::memcpy(removedPtr, lastPtr, componentSize);
      // Shrink the buffer by one component
      buffer.resize(buffer.size() - componentSize);
    }
    arch->entities.pop_back();
    // Update the moved entity's record to new index in this archetype
    entityLocations[lastEntity] = {arch, index};
  }

  template <typename... Components> friend class Query;
};

template <typename... Components> class Query {
  // Marker type for detecting the end of iteration.
  struct sentinel {};

  // We're calling a std::tuple<Entity, Components*...> where each pointer is
  // the relevant component data for current entity a "packet".
  typedef std::tuple<Entity, Components *...> Packet;

  // Archetype iterator
  std::vector<Archetype>::iterator archIt;
  std::vector<Archetype>::iterator archEnd;

  // The current entity in current archetype
  size_t entityIdx = 0;

  // Components an archetype is required to provide to satisfy query.
  std::vector<ComponentTypeID> required;

  // Current packet of valid data
  std::optional<Packet> current;

  // Return the next valid entity packet or none
  std::optional<Packet> fetchNext() {

    // Find next archetype with requested components
    for (/* this->archIt */; archIt < archEnd; archIt++) {
      Archetype &arch = *archIt;

      // Skip archetype If arch doesn't have all required components
      // TODO: this should probably be done with bitsets
      if (!std::all_of(required.begin(), required.end(),
                       [&arch](ComponentTypeID type) {
                         return std::binary_search(arch.signature.begin(),
                                                   arch.signature.end(), type);
                       })) {
        continue;
      }

      // Iterate through all entities in this archetype
      size_t entityCount = arch.entities.size();
      if (entityIdx < entityCount) {
        // We've found a valid entity in a matching archetype
        return {buildPacket(arch)};
      }

			// Moving to next archetype, reset entity index;
      entityIdx = 0;
    }

    return std::nullopt;
  }

  Packet buildPacket(Archetype &arch) {
    // The current entity ID in this archetype
    Entity entityId = arch.entities[entityIdx];

    // Helper lambda that returns a pointer to the component data.
    auto getPtr = [this, &arch](ComponentTypeID typeID) -> void * {
      // The raw byte buffer for this component type
      auto &buf = arch.data[typeID];
      // Components should all be the same size, so
      // component size = total bytes / entity count
      size_t entityCount = arch.entities.size();
      size_t compSize = buf.size() / entityCount;
      return static_cast<void *>(buf.data() + entityIdx * compSize);
    };

    // Expand the parameter pack to build the pointer set
    return {entityId, static_cast<Components *>(
                          getPtr(componentTypeID<Components>()))...};
  }

public:
  explicit Query(ECS &ecs)
      : archIt(ecs.archetypes.begin()), archEnd(ecs.archetypes.end()) {
    required = {componentTypeID<Components>()...};
    current = fetchNext();
  }

  auto begin() { return *this; }
  auto end() { return sentinel{}; }

  // If we couldn't fetch the last value we've exhausted the iterator.
  bool operator==(sentinel) const { return !current.has_value(); }
  bool operator!=(sentinel s) const { return !(*this == s); }

  // Pre-increment: prime the interator, then fetch the next.
  Query &operator++() {
    entityIdx++; // without this we'll just stay on current value.
    current = fetchNext();
    return *this;
  }

  // Post-increment: returns the current iterator state after increment.
  Query operator++(int) {
    auto prev = *this;
    ++*this;
    return prev;
  }

  // Assumes that we've already checked that val.has_value(). Any C++ `for
  // (auto &... : iterator)` will have checked through a
  const std::tuple<Entity, Components *...> &operator*() const {
    return *current;
  }
};

// Define sample components
struct Position {
  float x, y;
};
struct Velocity {
  float vx, vy;
};
struct Thing {
  bool a;
};

int main(int argc, char *argv[]) {
  ECS ecs;

  // Create an entity with Position and Velocity components
  Entity e1 = ecs.createEntity(Position{0.0f, 0.0f}, Velocity{1.0f, 0.5f});
  Entity e2 = ecs.createEntity(Position{5.0f, -3.0f}, Velocity{-1.0f, 0.0f});
  Entity e3 = ecs.createEntity(Position{15.0f, 5.0f}, Velocity{4.5f, -2.0f},
                               Thing{true});

  std::cout << "Query: pos vel \n";

  for (auto &[entityId, position, velocity] : ecs.query<Position, Velocity>()) {
    std::cout << entityId << ", ";
    std::cout << "{.x=" << position->x << ", .y=" << position->y << "}" << ", ";
    std::cout << "{.vx=" << velocity->vx << ", .vy=" << velocity->vy << "}"
              << std::endl;
  }

  std::cout << "Query: pos vel thing \n";

  for (auto &[entityId, position, velocity, thing] :
       ecs.query<Position, Velocity, Thing>()) {
    std::cout << entityId << ", ";
    std::cout << "{.x=" << position->x << ", .y=" << position->y << "}" << ", ";
    std::cout << "{.vx=" << velocity->vx << ", .vy=" << velocity->vy << "}"
              << ", ";
    std::cout << "{.x=" << thing->a << "}" << std::endl;
  }

  return 0;
}
