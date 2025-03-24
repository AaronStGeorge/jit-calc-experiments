#ifndef ECS_H
#define ECS_H

#include <bitset>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <optional>
#include <tuple>
#include <unordered_map>
#include <vector>

#include <llvm/ADT/SmallVector.h>

static constexpr std::size_t MAX_COMPONENTS = 128;

using Entity = uint32_t;
using ComponentTypeID = uint32_t;

// ----------------------------------------------------------------------
//  ComponentSignature
// ----------------------------------------------------------------------

// Signature<T> Class - Extends std::bitset<T> with an iterator
template <size_t Size> class Signature : public std::bitset<Size> {
public:
  class iterator {
  public:
    // Standard iterator typedefs
    using iterator_category = std::input_iterator_tag;
    using value_type = ComponentTypeID;
    using difference_type = std::ptrdiff_t;
    using pointer = const ComponentTypeID *;
    using reference = const ComponentTypeID &;

    iterator(const Signature &signature, size_t idx)
        : signature(signature), idx(idx) {
      skipCleared();
    }

    // Pre-increment
    iterator &operator++() {
      ++idx;
      skipCleared();
      return *this;
    }

    // Post-increment
    iterator operator++(int) {
      iterator temp = *this;
      ++(*this);
      return temp;
    }

    // Equality / inequality
    bool operator==(const iterator &other) const {
      return (signature == other.signature) && (idx == other.idx);
    }
    bool operator!=(const iterator &other) const { return !(*this == other); }

    // Dereference: returns the index of the set bit
    std::size_t operator*() const { return idx; }

  private:
    // Skip any cleared bits until we find a set bit or reach end
    void skipCleared() {
      while (idx < Size && !signature.test(idx)) {
        ++idx;
      }
    }

    const Signature &signature; // pointer to the signature (bitset)
    ComponentTypeID idx;        // current bit position
  };

  iterator begin() const { return iterator(*this, 0); }
  iterator end() const { return iterator(*this, Size); }
};

// A common alias for ECS usage: Signatures with up to MAX_COMPONENTS bits
using ComponentSignature = Signature<MAX_COMPONENTS>;

// ----------------------------------------------------------------------
//  Archetype
// ----------------------------------------------------------------------

struct Archetype {
  ComponentSignature signature; // Which components are included
  std::vector<Entity> entities; // Entities in this archetype

  // Map from component type to type erased storage for components of that type.
  std::unordered_map<ComponentTypeID, std::vector<uint8_t>> data;

  explicit Archetype(ComponentSignature signature);
};

// ----------------------------------------------------------------------
// Utilities
// ----------------------------------------------------------------------

// Declaration of a single global function that returns new IDs each time.
// This will be defined in the .cpp to ensure exactly one shared counter.
ComponentTypeID generateComponentTypeID();

// Returns a unique ID for each distinct component type.
template <typename Component> ComponentTypeID componentTypeID() {
  // Each distinct Component type has its own static local "typeID".
  static ComponentTypeID typeID = generateComponentTypeID();
  return typeID;
}

// Build a signature for a parameter pack of component types
template <typename... Components> ComponentSignature buildSignature() {
  ComponentSignature signature;
  (signature.set(componentTypeID<Components>()), ...);
  return signature;
}

// Tracks an entity's location in an archetype (which archetype + index).
struct EntityRecord {
  Archetype *archetype;
  std::size_t index;
};

// ----------------------------------------------------------------------
// ECS
// ----------------------------------------------------------------------
// Forward-declare the Query
template <typename... Components> class Query;

class ECS {
public:
  // Creates a new entity with a given set of components
  template <typename... Components> Entity createEntity(Components &&...comps) {
    ComponentSignature signature = buildSignature<Components...>();

    // Find or create an archetype with that signature
    Archetype *arch = findOrCreateArchetype(signature);

    // Register entity
    Entity entity = nextEntity++;
    std::size_t index = arch->entities.size();
    arch->entities.push_back(entity);
    entityLocations[entity] = {arch, index};

    // Store each component's bytes
    (storeComponentData(arch, componentTypeID<Components>(), &comps,
                        sizeof(Components)),
     ...);

    return entity;
  }

  // Add a component to an existing entity (migrating it to a new archetype if
  // needed)
  template <typename Component>
  void addComponent(Entity entity, Component &&comp) {
    EntityRecord record = entityLocations[entity];
    Archetype *oldArchetype = record.archetype;
    std::size_t oldIdx = record.index;

    // Build new signature = old + new bit
    ComponentSignature newSignature = oldArchetype->signature;
    ComponentTypeID newType = componentTypeID<Component>();
    if (newSignature.test(newType)) {
      // Nothing to do here
      return;
    }
    newSignature.set(newType);

    // Find/create new arch, move data
    Archetype *newArch = findOrCreateArchetype(newSignature);
    std::size_t newIndex = newArch->entities.size();
    newArch->entities.push_back(entity);

    // Copy old data over
    for (ComponentTypeID type : oldArchetype->signature) {
      copyComponentData(oldArchetype, newArch, type, oldIdx);
    }
    // Add the new component's data
    storeComponentData(newArch, newType, &comp, sizeof(Component));

    // Update record
    entityLocations[entity] = {newArch, newIndex};

    // Remove from old archetype
    removeEntityFromArchetype(oldArchetype, oldIdx, entity);
  }

  // Build a query for a set of components
  template <typename... Components> Query<Components...> query() {
    return Query<Components...>(*this);
  }

private:
  // Next entity ID to assign
  Entity nextEntity = 1;

  // All archetypes known to this ECS
  std::vector<Archetype> archetypes;

  // Map from entity ID -> (archetype, index)
  std::unordered_map<Entity, EntityRecord> entityLocations;

  // Creates or finds an existing archetype for 'signature'
  Archetype *findOrCreateArchetype(const ComponentSignature &signature);

  // Store bytes of a new component in the archetype
  void storeComponentData(Archetype *arch, ComponentTypeID type,
                          const void *compData, std::size_t compSize);

  // Copy existing component data from old -> new archetype
  void copyComponentData(Archetype *srcArch, Archetype *dstArch,
                         ComponentTypeID type, std::size_t srcIndex);

  // Remove an entity from an archetype
  void removeEntityFromArchetype(Archetype *arch, std::size_t index,
                                 Entity entity);

  // Let Query access ECS internals
  template <typename... Components> friend class Query;
};

// ----------------------------------------------------------------------
// Query
// ----------------------------------------------------------------------

template <typename... Components> class Query {
  // Which archetypes match the query signature. Using small vector to avoid
  // heap allocation for the common case.
  llvm::SmallVector<Archetype *, 128> matching_archetypes_;
  // Total number of entities in all matching archetypes
  std::size_t total_entities_ = 0;

  // We're calling a std::tuple<Entity, Components*...> where each pointer is
  // the relevant component data for current entity a "packet".
  typedef std::tuple<Entity, Components *...> Packet;

public:
  class iterator {
  public:
    // Iterator traits
    using iterator_category = std::random_access_iterator_tag;
    using value_type = Packet;
    using difference_type = std::ptrdiff_t;
    using pointer = Packet *;
    using reference = Packet &;

    explicit iterator(const Query<Components...> *q, size_t globalIdx = 0)
        : query(q), global_idx_(globalIdx) {
      if (globalIdx < query->total_entities_) {
        // Build the packet for the initial position
        setIndexesAndBuildPacket(globalIdx);
      }
    }

    // Dereference
    Packet &operator*() { return packet_; }
    const Packet &operator*() const { return packet_; }
    Packet *operator->() { return &packet_; }
    const Packet *operator->() const { return &packet_; }

    // Pre-increment
    iterator &operator++() {
      global_idx_++;
      if (global_idx_ >= query->total_entities_) {
        // Out of bounds
        return *this;
      }

      if (++entity_idx_ < arch_->entities.size()) {
        // Still within the current archetype
        packet_ = buildPacket(arch_, entity_idx_);
      } else {
        // Move to the next archetype
        setIndexesAndBuildPacket(global_idx_);
      }

      return *this;
    }
    // Post-increment
    iterator operator++(int) {
      iterator temp = *this;
      ++(*this);
      return temp;
    }

    // Pre-decrement
    iterator &operator--() {
      setIndexesAndBuildPacket(global_idx_ - 1);
      return *this;
    }
    // Post-decrement
    iterator operator--(int) {
      iterator temp = *this;
      --(*this);
      return temp;
    }

    // Random-access: +=
    iterator &operator+=(difference_type n) {
      setIndexesAndBuildPacket(global_idx_ + n);
      return *this;
    }
    // Random-access: -=
    iterator &operator-=(difference_type n) {
      setIndexesAndBuildPacket(global_idx_ - n);
      return *this;
    }

    // Random-access: it + n
    friend iterator operator+(const iterator &it, difference_type n) {
      return iterator(it.global_idx_ + n);
    }
    friend iterator operator+(difference_type n, const iterator &it) {
      return iterator(it.global_idx_ + n);
    }

    // Random-access: it - n
    friend iterator operator-(const iterator &it, difference_type n) {
      return iterator(it.global_idx_ - n);
    }

    // Random-access: difference between two iterators
    friend difference_type operator-(const iterator &lhs, const iterator &rhs) {
      return (lhs.global_idx_ - rhs.global_idx_);
    }

    // Comparison operators
    bool operator==(const iterator &other) const {
      return global_idx_ == other.global_idx_;
    }
    bool operator!=(const iterator &other) const {
      return global_idx_ != other.global_idx_;
    }
    bool operator<(const iterator &other) const {
      return global_idx_ < other.global_idx_;
    }
    bool operator>(const iterator &other) const {
      return global_idx_ > other.global_idx_;
    }
    bool operator<=(const iterator &other) const {
      return global_idx_ <= other.global_idx_;
    }
    bool operator>=(const iterator &other) const {
      return global_idx_ >= other.global_idx_;
    }

  private:
    const Query<Components...> *query;
    // Materialized packet.
    Packet packet_;
    // Global index into matching archetypes.
    size_t global_idx_;
    // Index into the current archetype.
    size_t entity_idx_;
    // Current archetype.
    Archetype *arch_;

    Packet buildPacket(Archetype *arch, size_t entityIdx) {
      // The current entity ID in this archetype
      Entity entityId = arch->entities[entityIdx];

      // Helper lambda that returns a pointer to the component data.
      auto getPtr = [arch, entityIdx](ComponentTypeID typeID) -> void * {
        // The raw byte buffer for this component type
        auto &buf = arch->data[typeID];
        // Components should all be the same size, so
        // component size = total bytes / entity count
        size_t entityCount = arch->entities.size();
        size_t compSize = buf.size() / entityCount;
        return static_cast<void *>(buf.data() + entityIdx * compSize);
      };

      // Expand the parameter pack to build the pointer set
      return {entityId, static_cast<Components *>(
                            getPtr(componentTypeID<Components>()))...};
    }

    // Based on new global index reset indexes into local archetypes and build
    // packet based on updated indexes.
    void setIndexesAndBuildPacket(size_t globalIdx) {
      if (globalIdx >= query->total_entities_) {
        // Out of bounds
        return;
      }

      this->global_idx_ = globalIdx;
      size_t idx = globalIdx;
      // Count entities in matching archetypes until we reach the global index.
      for (Archetype *matching_archetype : query->matching_archetypes_) {
        auto entityCount = matching_archetype->entities.size();
        if (idx < entityCount) {
          arch_ = matching_archetype;
          entity_idx_ = idx;
          packet_ = buildPacket(arch_, entity_idx_);
          return;
        }
        idx -= entityCount;
      }

      assert(true && "unreachable");
    }
  };

  explicit Query(ECS &ecs) {
    ComponentSignature required(buildSignature<Components...>());
    for (Archetype &arch : ecs.archetypes) {
      // Check if archetype has all required components
      if ((arch.signature & required) != required) {
        continue;
      }

      // We've found a matching archetype
      size_t entityCount = arch.entities.size();
      total_entities_ += entityCount;
      matching_archetypes_.push_back(&arch);
    }
  }

  iterator begin() const { return iterator(this, 0); }
  iterator end() const { return iterator(this, total_entities_); }

  size_t size() const { return total_entities_; }
};

#endif // ECS_H