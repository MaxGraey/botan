/*
* (C) 2014,2015 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#include "tests.h"

#if defined(BOTAN_HAS_HASH)
   #include <botan/hash.h>
#endif

namespace Botan_Tests {

#if defined(BOTAN_HAS_HASH)

namespace {

class Hash_Function_Tests final : public Text_Based_Test
   {
   public:
      Hash_Function_Tests() : Text_Based_Test("hash", "In,Out") {}

      std::vector<std::string> possible_providers(const std::string& algo) override
         {
         return provider_filter(Botan::HashFunction::providers(algo));
         }

      Test::Result run_one_test(const std::string& algo, const VarMap& vars) override
         {
         const std::vector<uint8_t> input    = get_req_bin(vars, "In");
         const std::vector<uint8_t> expected = get_req_bin(vars, "Out");

         Test::Result result(algo);

         const std::vector<std::string> providers = possible_providers(algo);

         if(providers.empty())
            {
            result.note_missing("hash " + algo);
            return result;
            }

         for(auto const& provider_ask : providers)
            {
            std::unique_ptr<Botan::HashFunction> hash(Botan::HashFunction::create(algo, provider_ask));

            if(!hash)
               {
               result.test_failure("Hash " + algo + " supported by " + provider_ask + " but not found");
               continue;
               }

            std::unique_ptr<Botan::HashFunction> clone(hash->clone());

            const std::string provider(hash->provider());
            result.test_is_nonempty("provider", provider);
            result.test_eq(provider, hash->name(), algo);
            result.test_eq(provider, hash->name(), clone->name());

            hash->update(input);
            result.test_eq(provider, "hashing", hash->final(), expected);

            clone->update(input);
            result.test_eq(provider, "hashing (clone)", clone->final(), expected);

            // Test to make sure clear() resets what we need it to
            hash->update("some discarded input");
            hash->clear();
            hash->update(nullptr, 0); // this should be effectively ignored
            hash->update(input);

            result.test_eq(provider, "hashing after clear", hash->final(), expected);

            if(input.size() > 5)
               {
               hash->update(input[0]);

               std::unique_ptr<Botan::HashFunction> fork = hash->copy_state();
               // verify fork copy doesn't affect original computation
               fork->update(&input[1], input.size() - 2);

               size_t so_far = 1;
               while(so_far < input.size())
                  {
                  size_t take = Test::rng().next_byte() % (input.size() - so_far);

                  if(input.size() - so_far == 1)
                     take = 1;

                  hash->update(&input[so_far], take);
                  so_far += take;
                  }
               result.test_eq(provider, "hashing split", hash->final(), expected);

               fork->update(&input[input.size() - 1], 1);
               result.test_eq(provider, "hashing split", fork->final(), expected);
               }

            if(hash->hash_block_size() > 0)
               {
               // GOST-34.11 uses 32 byte block
               result.test_gte("If hash_block_size is set, it is large", hash->hash_block_size(), 32);
               }
            }

         return result;
         }

   };

BOTAN_REGISTER_TEST("hash", Hash_Function_Tests);

}

#endif

}
