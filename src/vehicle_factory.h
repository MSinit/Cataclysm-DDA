#ifndef VEHICLE_GROUP_H
#define VEHICLE_GROUP_H

#include "json.h"
#include "mapgen.h"
#include <string>
#include <memory>
#include "weighted_list.h"

/**
 * This class is used to group vehicles together into groups in much the same way as
 *  item groups work.
 */
class VehicleGroup {
public:
    VehicleGroup() : vehicles() {}

    void add_vehicle(const vproto_id &type, const int &probability) {
        vehicles.add(type, probability);
    }

    const vproto_id &pick() const {
        return *(vehicles.pick());
    }

    static void load( JsonObject &jo );

private:
    weighted_int_list<vproto_id> vehicles;
};

using vgroup_id = string_id<VehicleGroup>;
extern std::unordered_map<vgroup_id, VehicleGroup> vgroups;

/**
 * The location and facing data needed to place a vehicle onto the map.
 */
 struct VehicleFacings {
    VehicleFacings(JsonObject &jo, const std::string &key);

    int pick() const {
        return values[rng(0, values.size()-1)];
    }

    std::vector<int> values;
};

struct VehicleLocation {
    VehicleLocation(const jmapgen_int &x, const jmapgen_int &y, const VehicleFacings &facings)
        : x(x), y(y), facings(facings) {}

    int pick_facing() const {
        return facings.pick();
    }

    point pick_point() const {
        return point(x.get(), y.get());
    }

    jmapgen_int x;
    jmapgen_int y;
    VehicleFacings facings;
};

/**
 * A list of vehicle locations which are valid for spawning new vehicles.
 */
struct VehiclePlacement {
    void add(const jmapgen_int &x, const jmapgen_int &y, const VehicleFacings &facings) {
        locations.emplace_back(x, y, facings);
    }

    const VehicleLocation* pick() const;
    static void load( JsonObject &jo );

    typedef std::vector<VehicleLocation> LocationMap;
    LocationMap locations;
};

using vplacement_id = string_id<VehiclePlacement>;

/**
 * These classes are used to wrap around a set of vehicle spawning functions. There are
 * currently two methods that can be used to spawn a vehicle. The first is by using a
 * c++ function. The second is by using data loaded from json.
 */

class VehicleFunction {
public:
    virtual ~VehicleFunction() { }
    virtual void apply(map& m, const std::string &terrainid) const = 0;
};

typedef void (*vehicle_gen_pointer)(map &m, const std::string &terrainid);

class VehicleFunction_builtin : public VehicleFunction {
public:
    VehicleFunction_builtin(const vehicle_gen_pointer &func) : func(func) {}
    ~VehicleFunction_builtin() { }

    void apply(map& m, const std::string &terrainid) const override {
        func(m, terrainid);
    }

private:
    vehicle_gen_pointer func;
};

class VehicleFunction_json : public VehicleFunction {
public:
    VehicleFunction_json(JsonObject &jo);
    ~VehicleFunction_json() { }

    void apply(map& m, const std::string &terrain_name) const override;

private:
    vgroup_id vehicle;
    jmapgen_int number;
    int fuel;
    int status;

    std::string placement;
    std::unique_ptr<VehicleLocation> location;
};

/**
 * This class handles a weighted list of different spawn functions, allowing a single
 * vehicle_spawn to have multiple possibilities.
 */
class VehicleSpawn;
using vspawn_id = string_id<VehicleSpawn>;

class VehicleSpawn {
public:
    void add(const double &weight, const std::shared_ptr<VehicleFunction> &func) {
        types.add(func, weight);
    }

    const VehicleFunction* pick() const {
        return types.pick()->get();
    }

    /**
     * This will invoke the vehicle spawn on the map.
     * @param m The map on which to add the vehicle.
     * @param terrain_name The name of the terrain being spawned on.
     */
    void apply(map& m, const std::string &terrain_name) const;

    /**
     * A static helper function. This will invoke the supplied vehicle spawn on the map.
     * @param id The spawnid to apply
     * @param m The map on which to add the vehicle.
     * @param terrain_name The name of the terrain being spawned on.
     */
    static void apply(const vspawn_id &id, map& m, const std::string &terrain_name);

    static void load( JsonObject &jo );

private:
    weighted_float_list<std::shared_ptr<VehicleFunction>> types;

    // builtin functions
    static void builtin_no_vehicles(map& m, const std::string &terrainid);
    static void builtin_jackknifed_semi(map& m, const std::string &terrainid);
    static void builtin_pileup(map& m, const std::string &terrainid);
    static void builtin_policepileup(map& m, const std::string &terrainid);

    typedef std::unordered_map<std::string, vehicle_gen_pointer> FunctionMap;
    static FunctionMap builtin_functions;
};

#endif
