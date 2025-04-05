#ifndef ECS_H
#define ECS_H

#include <array>
#include <bitset>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <optional>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include "llvm/ADT/STLExtras.h"
#include "llvm/Support/ErrorHandling.h"
#include <llvm/ADT/SmallVector.h>

static constexpr std::size_t MAX_COMPONENTS = 128;

using Entity = uint32_t;
using ComponentTypeID = uint32_t;
// We're calling a std::tuple<Entity, Components*...> where each pointer is
// the relevant component data for current entity a "packet".
template <typename... Components>
using Packet = std::tuple<Entity, Components *...>;

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

// Archetype holds a signature, entity list, and associated type-erased storage.
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

#define STATIC_ASSERT_POD(type)                                                \
  static_assert(std::is_trivial_v<type> && std::is_standard_layout_v<type>,    \
                #type " is not a POD type!");

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
// Evil template meta-programming bullshit
// ----------------------------------------------------------------------

// Check if T is exactly 'Entity'
template <typename T> using is_entity_t = std::is_same<T, Entity>;

// Determine if Container yields Entity or pair-of-entity
template <typename Container>
using container_value_t =
    typename std::decay_t<decltype(*std::begin(std::declval<Container>()))>;

// Is this a container of Entity types?
template <typename Container>
using is_container_of_entities = is_entity_t<container_value_t<Container>>;

// Check if T is a pair of Entity types
template <typename T> struct is_pair_of_entity : std::false_type {};
template <>
struct is_pair_of_entity<std::pair<Entity, Entity>> : std::true_type {};

// Is this a container of std::pair<Entity, Entity> types
template <typename Container>
using is_container_of_entity_pairs =
    is_pair_of_entity<container_value_t<Container>>;

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

  // Returns a Query object that scans archetypes to find entities having the
  // specified signature. Returned Query will iterate over Packets of the
  // specified component types.
  //
  // Example:
  //   for(auto &[entity, pos, vel]: ecs.query<Position, Velocity>()) {
  //     // entity is of type Entity
  //     // pos is of type Position *
  //     // vel is of type Velocity *
  //   }
  //
  // See note on SFINE bs on other constructor.
  template <typename... Components> Query<Components...> query() {
    return Query<Components...>(*this);
  }

  // Returns an iterable Query object raging over entities in container. It's
  // assumed (and asserted) that provided entities have the required signature.
  // This is useful for querying a known subset of entities rather than scanning
  // all.
  template <
      typename... Components, typename Container,
      std::enable_if_t<is_container_of_entities<Container>::value, int> = 0>
  Query<Components...> query(Container &&container) {
    return Query<Components...>(*this, std::forward<Container>(container));
  }

  // Build an iterator over pairs of Packets corresponding each pair of entities
  // in the provided container.
  //
  // Example:
  //   std::vector<std::pair<Entity, Entity>> overlaps;
  //   // fill overlaps ...
  //   for (auto [packetA, packetB] : ecs.query<Position, Velocity>(overlaps)) {
  //     auto &[e1, pos1, vel1] = packetA;
  //     auto &[e2, pos2, vel2] = packetB;
  //
  //     // e1 and e2 are of type Entity
  //     // pos1 and pos2 are of type Position *
  //     // vel1 and vel2 of of type Velocity *
  //
  // Note on SFINE (Substitution Failure Is Not An Error) bullshit: we only want
  // this constructor to be generated if the container is an iterable collection
  // of std::pair<Entity, Entity>. If the type is not correct, we want to
  // generate a malformed lump of code, that will cause the template engine to
  // just skip it (Substitution Failure Is Not An Error). We do that by handing
  // the compile time type produced by is_container_of_entity_pairs to
  // std::enable_if_t. If is_container_of_entity_pairs produces the correct
  // value we will end up stamping out `int = 0` (which is oddly valid)
  // otherwise it will stamp out `= 0` which is not valid and the substitution
  // will not be generated.
  template <
      typename... Components, typename Container,
      std::enable_if_t<is_container_of_entity_pairs<Container>::value, int> = 0>
  auto query(Container &&container) {
    return llvm::map_range(container, [this](std::pair<Entity, Entity> p) {
      // Create array because pair doesn't support iteration.
      std::array<Entity, 2> tmp{p.first, p.second};

      // Query to get the specified packets
      Query<Components...> tmpQuery = query<Components...>(tmp);

      // Repack into pair
      return std::pair<Packet<Components...>, Packet<Components...>>{
          *tmpQuery.begin(), *(tmpQuery.begin() + 1)};
    });
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

struct ArchetypeMask {
  llvm::SmallVector<Entity, 8> entities;
  llvm::SmallVector<size_t, 8> indexes;

  size_t size() const {
    assert(entities.size() == indexes.size() &&
           "entity and indexes should be 1 to 1");
    return entities.size();
  }
};

struct MaskedArchetype {
  // LocalIndex type exists to avoid confusing the index into MaskedArchetype
  // with an index into Archetype.
  class LocalIndex {
    friend MaskedArchetype;
    size_t index;

  public:
    LocalIndex() : index(0) {}
    explicit LocalIndex(size_t index) : index(index) {}

    // Pre-increment
    LocalIndex &operator++() {
      ++index;
      return *this;
    }
    // Post-increment
    LocalIndex operator++(int) {
      LocalIndex temp = *this;
      ++(*this);
      return temp;
    }

    // Comparison operators
    bool operator<(const size_t &other) const { return index < other; }
  };

  Archetype *archetype;
  // Entities to iterate in this archetype. Without a mask, iterate through
  // all the entities.
  std::optional<ArchetypeMask> mask;

  size_t size() const {
    if (mask) {
      return mask->size();
    }
    return archetype->entities.size();
  }

  size_t index(LocalIndex idx) {
    if (mask) {
      return (*mask).indexes[idx.index];
    }
    return idx.index;
  }

  size_t entity(LocalIndex idx) {
    if (mask) {
      return (*mask).entities[idx.index];
    }
    return archetype->entities[idx.index];
  }
};

// Query objects let us iterate over matching entities and their components.
template <typename... Components> class Query {
  // Which archetypes match the query signature. Using small vector to avoid
  // heap allocation for the common case.
  llvm::SmallVector<MaskedArchetype, 128> matchingArchetypes;
  // Total number of entities in all matching archetypes
  std::size_t totalEntityCount = 0;

public:
  class iterator {
  public:
    // Iterator traits
    using iterator_category = std::random_access_iterator_tag;
    using value_type = Packet<Components...>;
    using difference_type = std::ptrdiff_t;
    using pointer = Packet<Components...> *;
    using reference = Packet<Components...> &;

    iterator(const Query<Components...> *query, size_t globalIdx)
        : query(query), globalIdx(globalIdx) {
      if (globalIdx < query->totalEntityCount) {
        // Build the packet for the initial position
        setIndexesAndBuildPacket(globalIdx);
      }
    }

    // Dereference
    Packet<Components...> &operator*() { return packet; }
    const Packet<Components...> &operator*() const { return packet; }
    Packet<Components...> *operator->() { return &packet; }
    const Packet<Components...> *operator->() const { return &packet; }

    // Pre-increment
    iterator &operator++() {
      globalIdx++;
      if (globalIdx >= query->totalEntityCount) {
        // Out of bounds
        return *this;
      }

      assert(currentArchetype.has_value() &&
             "operator++ called without valid arch");
      if (++localIdx < currentArchetype.value().size()) {
        // Still within the current archetype
        setPacket();
      } else {
        // Move to the next archetype
        setIndexesAndBuildPacket(globalIdx);
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
      setIndexesAndBuildPacket(globalIdx - 1);
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
      setIndexesAndBuildPacket(globalIdx + n);
      return *this;
    }
    // Random-access: -=
    iterator &operator-=(difference_type n) {
      setIndexesAndBuildPacket(globalIdx - n);
      return *this;
    }

    // Random-access: it + n
    friend iterator operator+(const iterator &it, difference_type n) {
      return iterator(it.query, it.globalIdx + n);
    }
    friend iterator operator+(difference_type n, const iterator &it) {
      return iterator(it.query, it.globalIdx + n);
    }

    // Random-access: it - n
    friend iterator operator-(const iterator &it, difference_type n) {
      return iterator(it.query, it.globalIdx - n);
    }

    // Random-access: difference between two iterators
    friend difference_type operator-(const iterator &lhs, const iterator &rhs) {
      return (lhs.globalIdx - rhs.globalIdx);
    }

    // Comparison operators
    bool operator==(const iterator &other) const {
      return globalIdx == other.globalIdx;
    }
    bool operator!=(const iterator &other) const {
      return globalIdx != other.globalIdx;
    }
    bool operator<(const iterator &other) const {
      return globalIdx < other.globalIdx;
    }
    bool operator>(const iterator &other) const {
      return globalIdx > other.globalIdx;
    }
    bool operator<=(const iterator &other) const {
      return globalIdx <= other.globalIdx;
    }
    bool operator>=(const iterator &other) const {
      return globalIdx >= other.globalIdx;
    }

  private:
    const Query<Components...> *query;
    // Materialized packet.
    Packet<Components...> packet;
    // Global index into to the total entity count. For example if we have 2
    // matching archetypes with 10 entities each, this index may be 15.
    size_t globalIdx;
    // Index into the current masked archetype. Note, this is not the index into
    // the archetype itself. For example, the archetype may have 16 entities but
    // our mask limits our view to 2 entities. The local index---into the masked
    // archetype---will be 0 and 1, but the index into the underlying archetype
    // could be 5 and 8.
    MaskedArchetype::LocalIndex localIdx;
    // Current masked archetype, localIdx attaches to this
    // archetype.
    std::optional<MaskedArchetype> currentArchetype = std::nullopt;

    void setPacket() {
      assert(currentArchetype.has_value() &&
             "setPacket called without valid arch");
      // Helper lambda that returns a pointer to the component data.
      auto getPtr = [this](ComponentTypeID typeID) -> void * {
        // The raw byte buffer for this component type
        auto &buf = currentArchetype.value().archetype->data[typeID];
        // Components should all be the same size, so
        // component size = total bytes / entity count
        size_t entityCount =
            currentArchetype.value().archetype->entities.size();
        size_t compSize = buf.size() / entityCount;
        size_t archIdx = currentArchetype.value().index(localIdx);
        return static_cast<void *>(buf.data() + archIdx * compSize);
      };

      // Expand the parameter pack to build the pointer set
      packet = {
          currentArchetype.value().entity(localIdx),
          static_cast<Components *>(getPtr(componentTypeID<Components>()))...};
    }

    // Based on new global index reset indexes into local archetypes and build
    // packet based on updated indexes.
    void setIndexesAndBuildPacket(size_t newGlobalIndex) {
      if (newGlobalIndex >= query->totalEntityCount) {
        // Out of bounds
        return;
      }

      this->globalIdx = newGlobalIndex;
      size_t tmpIdx = newGlobalIndex;
      // Count entities in matching archetypes until we reach the global index.
      for (const MaskedArchetype &arch : query->matchingArchetypes) {
        size_t entityCount = arch.size();
        if (tmpIdx < entityCount) {
          currentArchetype = arch;
          localIdx = MaskedArchetype::LocalIndex(tmpIdx);
          setPacket();
          return;
        }
        tmpIdx -= entityCount;
      }

      llvm_unreachable("Index out of bounds");
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
      totalEntityCount += entityCount;
      matchingArchetypes.push_back({&arch, std::nullopt});
    }
  }

  template <typename Container> Query(ECS &ecs, Container &&entities) {
    ComponentSignature required(buildSignature<Components...>());

    totalEntityCount = entities.size();
    for (Entity entity : entities) {
      assert(ecs.entityLocations.find(entity) != ecs.entityLocations.end() &&
             "entity doesn't have associated archetype");
      EntityRecord &entityRecord = ecs.entityLocations[entity];

      // Sanity check if archetype has all required components
      assert((entityRecord.archetype->signature & required) == required &&
             "entity doesn't have required signature");

      // Compact if possible.
      if (!matchingArchetypes.empty() &&
          matchingArchetypes.back().archetype == entityRecord.archetype) {
        ArchetypeMask &mask = *matchingArchetypes.back().mask;
        mask.entities.push_back(entity);
        mask.indexes.push_back(entityRecord.index);
        continue;
      }

      // This entity comes from a new archetype, create new container.
      matchingArchetypes.push_back(MaskedArchetype{
          .archetype = entityRecord.archetype,
          .mask = std::make_optional<ArchetypeMask>({
              .entities = {entity},
              .indexes = {entityRecord.index},
          }),
      });
    }
  }

  iterator begin() const { return iterator(this, 0); }
  iterator end() const { return iterator(this, totalEntityCount); }

  size_t size() const { return totalEntityCount; }
};

#endif // ECS_H