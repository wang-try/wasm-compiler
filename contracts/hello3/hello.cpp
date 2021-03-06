#include <cosiolib/contract.hpp>
#include <cosiolib/print.hpp>

using timestamp_t = uint64_t;

// the database table record type
struct greeting {
    cosio::name name;
    uint32_t count;
    timestamp_t last_seen;
    
    // record type must support serialization.
    COSIO_SERIALIZE(greeting, (name)(count)(last_seen))
};

// the singleton record type, which must be derived from cosio::singleton_record.
struct stats : public cosio::singleton_record {
    stats(): users(0), visits(0) {}

    uint32_t users;
    uint32_t visits;
    
    // record type must support serialization.
    COSIO_SERIALIZE_DERIVED(stats, cosio::singleton_record, (users)(visits))
};

// the contract class
class hello : public cosio::contract {
public:
    using cosio::contract::contract;

    void hi( cosio::name user ) {
        
        // load the global counter
        counter.get_or_create();
        
        // check if we have greeted this guy before
        if(!table_greetings.has(user)) {
            // first time meeting
            table_greetings.insert([&](greeting& r) {
                r.name = user;
                r.count = 1;
                r.last_seen = cosio::current_timestamp();
            });
            counter.update([&](stats& s) {
                s.users++;
                s.visits++;
            });
        } else {
            // increase the greeting counter
            table_greetings.update(user, [&](greeting& r) {
                r.count++;
                r.last_seen = cosio::current_timestamp();
            });
            counter.update([&](stats& s) {
                s.visits++;
            });
        }
        // print something
        auto r = table_greetings.get(user);
        auto s = counter.get();
        cosio::print_f("Hello %, we have met % times. I have greeted % persons, % greetings in total.\n", user.string(), r.count, s.users, s.visits);

        // execute a contract. this contract's add(123456, 789).
        cosio::execute_contract( 
            this->get_name(),                     // name of target contract
            "add",                            // name of target method
            0,                                // amount of coins to transfer from current contract to target contract
            123456, 789                       // variadic parameters for target method
            );

        cosio::print("Cool, I just called another contract :)\n");
    }

    void add(int32_t a, int32_t b) {
        cosio::print_f("sum of % and % is %\n", a, b, a + b);
    }

private:
    //
    // define a class data member named "table_greetings" representing the database table which,
    // - has name "table_greetings", the same as variable name
    // - has a record type of greeting
    // - takes greeting::name as primary key
    // - has 2 secondary indices taking greeting::count and greeting::last_seen as indexing fields respectively
    //
    COSIO_DEFINE_TABLE( table_greetings, greeting, (name)(count)(last_seen) );
    
    //
    // define a class data member named "table_hello" representing the database table which,
    // - has name "hello"
    // - has a record type of greeting
    // - takes greeting::name as primary key
    // - has no secondary indices
    //
    COSIO_DEFINE_NAMED_TABLE( table_hello, "hello", greeting, (name) );
    
    //
    // define a class data member named "counter" representing a singleton table which,
    // - has name "global_counters"
    // - has a record type of stats
    //
    COSIO_DEFINE_NAMED_SINGLETON( counter, "global_counters", stats );
};

// declare that this contract is named "hello", and has 2 methods named "hi", "add".
COSIO_ABI(hello, (hi)(add))
