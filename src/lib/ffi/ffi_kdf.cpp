/*
* (C) 2015,2017 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#include <botan/ffi.h>
#include <botan/internal/ffi_util.h>
#include <botan/internal/ffi_rng.h>
#include <botan/pbkdf.h>
#include <botan/kdf.h>

#if defined(BOTAN_HAS_BCRYPT)
  #include <botan/bcrypt.h>
#endif

#if defined(BOTAN_HAS_SCRYPT)
  #include <botan/scrypt.h>
#endif

extern "C" {

using namespace Botan_FFI;

int botan_pbkdf(const char* algo, uint8_t out[], size_t out_len,
                const char* pass, const uint8_t salt[], size_t salt_len,
                size_t iterations)
   {
   return botan_pwdhash(algo,
                        iterations,
                        0,
                        0,
                        out, out_len,
                        pass, std::strlen(pass),
                        salt, salt_len);
   }

int botan_pbkdf_timed(const char* algo,
                      uint8_t out[], size_t out_len,
                      const char* password,
                      const uint8_t salt[], size_t salt_len,
                      size_t ms_to_run,
                      size_t* iterations_used)
   {
   return botan_pwdhash_timed(algo,
                              static_cast<uint32_t>(ms_to_run),
                              iterations_used,
                              nullptr,
                              nullptr,
                              out, out_len,
                              password, std::strlen(password),
                              salt, salt_len);
   }

int botan_pwdhash(
   const char* algo,
   size_t param1,
   size_t param2,
   size_t param3,
   uint8_t out[],
   size_t out_len,
   const char* password,
   size_t password_len,
   const uint8_t salt[],
   size_t salt_len)
   {
   return ffi_guard_thunk(BOTAN_CURRENT_FUNCTION, [=]() -> int {
      auto pwdhash_fam = Botan::PasswordHashFamily::create(algo);

      if(!pwdhash_fam)
         return BOTAN_FFI_ERROR_NOT_IMPLEMENTED;

      auto pwdhash = pwdhash_fam->from_params(param1, param2, param3);

      pwdhash->derive_key(out, out_len,
                          password, password_len,
                          salt, salt_len);

      return BOTAN_FFI_SUCCESS;
      });
   }

int botan_pwdhash_timed(
   const char* algo,
   uint32_t msec,
   size_t* param1,
   size_t* param2,
   size_t* param3,
   uint8_t out[],
   size_t out_len,
   const char* password,
   size_t password_len,
   const uint8_t salt[],
   size_t salt_len)
   {
   return ffi_guard_thunk(BOTAN_CURRENT_FUNCTION, [=]() -> int {

      auto pwdhash_fam = Botan::PasswordHashFamily::create(algo);

      if(!pwdhash_fam)
         return BOTAN_FFI_ERROR_NOT_IMPLEMENTED;

      auto pwdhash = pwdhash_fam->tune(out_len, std::chrono::milliseconds(msec));

      if(param1)
         *param1 = pwdhash->iterations();
      if(param2)
         *param2 = pwdhash->parallelism();
      if(param3)
         *param3 = pwdhash->memory_param();

      pwdhash->derive_key(out, out_len,
                          password, password_len,
                          salt, salt_len);

      return BOTAN_FFI_SUCCESS;
      });
   }

int botan_kdf(const char* kdf_algo,
              uint8_t out[], size_t out_len,
              const uint8_t secret[], size_t secret_len,
              const uint8_t salt[], size_t salt_len,
              const uint8_t label[], size_t label_len)
   {
   return ffi_guard_thunk(BOTAN_CURRENT_FUNCTION, [=]() -> int {
      std::unique_ptr<Botan::KDF> kdf(Botan::get_kdf(kdf_algo));
      kdf->kdf(out, out_len, secret, secret_len, salt, salt_len, label, label_len);
      return BOTAN_FFI_SUCCESS;
      });
   }

int botan_scrypt(uint8_t out[], size_t out_len,
                 const char* password,
                 const uint8_t salt[], size_t salt_len,
                 size_t N, size_t r, size_t p)
   {
#if defined(BOTAN_HAS_SCRYPT)
   return ffi_guard_thunk(BOTAN_CURRENT_FUNCTION, [=]() -> int {
      Botan::scrypt(out, out_len, password, strlen(password), salt, salt_len, N, r, p);
      return BOTAN_FFI_SUCCESS;
   });
#else
   return BOTAN_FFI_ERROR_NOT_IMPLEMENTED;
#endif
   }

int botan_bcrypt_generate(uint8_t* out, size_t* out_len,
                          const char* pass,
                          botan_rng_t rng_obj, size_t wf,
                          uint32_t flags)
   {
#if defined(BOTAN_HAS_BCRYPT)
   return ffi_guard_thunk(BOTAN_CURRENT_FUNCTION, [=]() -> int {
      if(out == nullptr || out_len == nullptr || pass == nullptr)
         return BOTAN_FFI_ERROR_NULL_POINTER;

      if(flags != 0)
         return BOTAN_FFI_ERROR_BAD_FLAG;

      if(wf < 4 || wf > 18)
         return BOTAN_FFI_ERROR_BAD_PARAMETER;

      Botan::RandomNumberGenerator& rng = safe_get(rng_obj);
      const std::string bcrypt = Botan::generate_bcrypt(pass, rng, static_cast<uint16_t>(wf));
      return write_str_output(out, out_len, bcrypt);
      });
#else
   return BOTAN_FFI_ERROR_NOT_IMPLEMENTED;
#endif
   }

int botan_bcrypt_is_valid(const char* pass, const char* hash)
   {
#if defined(BOTAN_HAS_BCRYPT)
   return ffi_guard_thunk(BOTAN_CURRENT_FUNCTION, [=]() -> int {
      return Botan::check_bcrypt(pass, hash) ? BOTAN_FFI_SUCCESS : BOTAN_FFI_INVALID_VERIFIER;
      });
#else
   return BOTAN_FFI_ERROR_NOT_IMPLEMENTED;
#endif
   }

}
