/*  ---------------------------------------------------------------------------

    (c) copyright 2022 Altair Semiconductor, Ltd. All rights reserved.

    This software, in source or object form (the "Software"), is the
    property of Altair Semiconductor Ltd. (the "Company") and/or its
    licensors, which have all right, title and interest therein, You
    may use the Software only in  accordance with the terms of written
    license agreement between you and the Company (the "License").
    Except as expressly stated in the License, the Company grants no
    licenses by implication, estoppel, or otherwise. If you are not
    aware of or do not agree to the License terms, you may not use,
    copy or modify the Software. You may use the source code of the
    Software only for your internal purposes and may not distribute the
    source code of the Software, any part thereof, or any derivative work
    thereof, to any third party, except pursuant to the Company's prior
    written consent.
    The Software is the confidential information of the Company.

   ------------------------------------------------------------------------- */

#include "certificates.h"

#include <string.h>

#include "altcom_certmgmt.h"

char *GlobalSign_Root_CA_name = "GlobalSign_Root_CA.crt";
char *GlobalSign_Root_CA =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIDdTCCAl2gAwIBAgILBAAAAAABFUtaw5QwDQYJKoZIhvcNAQEFBQAwVzELMAkG\n"
    "A1UEBhMCQkUxGTAXBgNVBAoTEEdsb2JhbFNpZ24gbnYtc2ExEDAOBgNVBAsTB1Jv\n"
    "b3QgQ0ExGzAZBgNVBAMTEkdsb2JhbFNpZ24gUm9vdCBDQTAeFw05ODA5MDExMjAw\n"
    "MDBaFw0yODAxMjgxMjAwMDBaMFcxCzAJBgNVBAYTAkJFMRkwFwYDVQQKExBHbG9i\n"
    "YWxTaWduIG52LXNhMRAwDgYDVQQLEwdSb290IENBMRswGQYDVQQDExJHbG9iYWxT\n"
    "aWduIFJvb3QgQ0EwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQDaDuaZ\n"
    "jc6j40+Kfvvxi4Mla+pIH/EqsLmVEQS98GPR4mdmzxzdzxtIK+6NiY6arymAZavp\n"
    "xy0Sy6scTHAHoT0KMM0VjU/43dSMUBUc71DuxC73/OlS8pF94G3VNTCOXkNz8kHp\n"
    "1Wrjsok6Vjk4bwY8iGlbKk3Fp1S4bInMm/k8yuX9ifUSPJJ4ltbcdG6TRGHRjcdG\n"
    "snUOhugZitVtbNV4FpWi6cgKOOvyJBNPc1STE4U6G7weNLWLBYy5d4ux2x8gkasJ\n"
    "U26Qzns3dLlwR5EiUWMWea6xrkEmCMgZK9FGqkjWZCrXgzT/LCrBbBlDSgeF59N8\n"
    "9iFo7+ryUp9/k5DPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNVHRMBAf8E\n"
    "BTADAQH/MB0GA1UdDgQWBBRge2YaRQ2XyolQL30EzTSo//z9SzANBgkqhkiG9w0B\n"
    "AQUFAAOCAQEA1nPnfE920I2/7LqivjTFKDK1fPxsnCwrvQmeU79rXqoRSLblCKOz\n"
    "yj1hTdNGCbM+w6DjY1Ub8rrvrTnhQ7k4o+YviiY776BQVvnGCv04zcQLcFGUl5gE\n"
    "38NflNUVyRRBnMRddWQVDf9VMOyGj/8N7yy5Y0b2qvzfvGn9LhJIZJrglfCm7ymP\n"
    "AbEVtQwdpf5pLGkkeB6zpxxxYu7KyJesF12KwvhHhm4qxFYxldBniYUr+WymXUad\n"
    "DKqC5JlR3XC321Y9YeRq4VzW9v493kHMB65jUr9TU/Qr6cf9tveCX4XSQRjbgbME\n"
    "HMUfpIBvFSDJ3gyICh3WZlXi/EjJKSZp4A==\n"
    "-----END CERTIFICATE-----\n";

char *californium_cert_name = "californium_cert.pem";
char *californium_cert =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIB/jCCAaSgAwIBAgIJAJ3+bXCmfffGMAoGCCqGSM49BAMCMFoxDjAMBgNVBAMT\n"
    "BWNmLWNhMRQwEgYDVQQLEwtDYWxpZm9ybml1bTEUMBIGA1UEChMLRWNsaXBzZSBJ\n"
    "b1QxDzANBgNVBAcTBk90dGF3YTELMAkGA1UEBhMCQ0EwHhcNMjIxMDI2MDcwMzMx\n"
    "WhcNMjMxMDI2MDcwMzMxWjBeMRIwEAYDVQQDEwljZi1jbGllbnQxFDASBgNVBAsT\n"
    "C0NhbGlmb3JuaXVtMRQwEgYDVQQKEwtFY2xpcHNlIElvVDEPMA0GA1UEBxMGT3R0\n"
    "YXdhMQswCQYDVQQGEwJDQTBZMBMGByqGSM49AgEGCCqGSM49AwEHA0IABNcwrXwo\n"
    "b76WQSIQjt398EYq6hIQrY6PtWP9I0g48vlLirtzE4v1c0rqPy/M2v4INNttLMtb\n"
    "QvanSuyornAUvoyjTzBNMB0GA1UdDgQWBBTz/KiBShT/0ALlH+PnE1WEEHAVjTAL\n"
    "BgNVHQ8EBAMCB4AwHwYDVR0jBBgwFoAUWTm+jUaqsoI0wrlDeHiybI33f+cwCgYI\n"
    "KoZIzj0EAwIDSAAwRQIgfIRjDPUDTlQD/QhhKZvabsvb6vSM26aOaJczP2MCsq0C\n"
    "IQD3UDAcYWtVuxENn2YIqBUWTIOkNMXZfMPIH4Khc/0xSA==\n"
    "-----END CERTIFICATE-----\n";

char *californium_key_name = "californium_key.pem";
char *californium_key =
    "-----BEGIN PRIVATE KEY-----\n"
    "MEECAQAwEwYHKoZIzj0CAQYIKoZIzj0DAQcEJzAlAgEBBCDJXqiqopBm7fDmo5Ak\n"
    "Rwv5SYDgmB2US2VvwKQQhXkISA==\n"
    "-----END PRIVATE KEY-----\n";

char *californium_root_name = "californium_root.pem";
char *californium_root =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIB3jCCAYSgAwIBAgIJALd+Edoht/I5MAoGCCqGSM49BAMCMFwxEDAOBgNVBAMT\n"
    "B2NmLXJvb3QxFDASBgNVBAsTC0NhbGlmb3JuaXVtMRQwEgYDVQQKEwtFY2xpcHNl\n"
    "IElvVDEPMA0GA1UEBxMGT3R0YXdhMQswCQYDVQQGEwJDQTAeFw0yMjEwMjYwNzAz\n"
    "MjRaFw0yMzEwMjYwNzAzMjRaMFwxEDAOBgNVBAMTB2NmLXJvb3QxFDASBgNVBAsT\n"
    "C0NhbGlmb3JuaXVtMRQwEgYDVQQKEwtFY2xpcHNlIElvVDEPMA0GA1UEBxMGT3R0\n"
    "YXdhMQswCQYDVQQGEwJDQTBZMBMGByqGSM49AgEGCCqGSM49AwEHA0IABH6oGChw\n"
    "1sNLAP+0FsSWWjt4Gq8gQGCfYOxfUg/R78PyQlsd7wN2fw1JzRHhKk5/E7w0PeiU\n"
    "mi7qeeA3rsZnTwKjLzAtMB0GA1UdDgQWBBRYzQRIqIulcix6HZligx+3SXSDlzAM\n"
    "BgNVHRMEBTADAQH/MAoGCCqGSM49BAMCA0gAMEUCIGpzyol3O/vrXaVD1xbWgMWy\n"
    "2qxknwDE0PIPlhFTJ+EgAiEA1ZsyEMBp6DqWYQVxr2h40P3PGKN7mNYdHQUq9OJx\n"
    "ow0=\n"
    "-----END CERTIFICATE-----\n";

static int write_certificate(const char *credName, uint8_t *credData, uint16_t credLen) {
  if (altcom_WriteCredential(CREDTYPE_CERT, credName, credData, credLen) != CERTMGMT_SUCCESS) {
    printf(" failed\n  !  error writing certificate\n\n%s\n", credName);
    return -1;
  }

  return 0;
}

static int write_privkey(const char *credName, uint8_t *credData, uint16_t credLen) {
  if (altcom_WriteCredential(CREDTYPE_PRIVKEY, credName, credData, credLen) != CERTMGMT_SUCCESS) {
    printf(" failed\n  !  error writing private key\n\n%s\n", credName);
    return -1;
  }

  return 0;
}

int install_certificates(char *s) {
  write_certificate(GlobalSign_Root_CA_name, (uint8_t *)GlobalSign_Root_CA,
                    strlen(GlobalSign_Root_CA));
  write_certificate(californium_root_name, (uint8_t *)californium_root, strlen(californium_root));

  write_certificate(californium_cert_name, (uint8_t *)californium_cert, strlen(californium_cert));
  write_privkey(californium_key_name, (uint8_t *)californium_key, strlen(californium_key));

  return 0;
}

int uninstall_certificates(char *s) {
  altcom_DeleteCredential(GlobalSign_Root_CA_name);
  altcom_DeleteCredential(californium_root_name);
  altcom_DeleteCredential(californium_cert_name);
  altcom_DeleteCredential(californium_key_name);

  return 0;
}
