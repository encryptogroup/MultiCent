Certificates generated as follows:

```sh
openssl req -x509 -newkey rsa:4096 -keyout key_ca.pem -out cert_ca.pem -sha256 -days 50000 -nodes -subj "/C=XX/ST=StateName/L=CityName/O=CompanyName/OU=CompanySectionName/CN=CommonNameOrHostname"
openssl req -new -newkey rsa:4096 -keyout key1.pem -out cert1_req.pem -nodes -subj "/C=XX/ST=StateName/L=CityName/O=CompanyName/OU=CompanySectionName/CN=CommonNameOrHostname"
openssl req -new -newkey rsa:4096 -keyout key2.pem -out cert2_req.pem -nodes -subj "/C=XX/ST=StateName/L=CityName/O=CompanyName/OU=CompanySectionName/CN=CommonNameOrHostname"
openssl req -new -newkey rsa:4096 -keyout key3.pem -out cert3_req.pem -nodes -subj "/C=XX/ST=StateName/L=CityName/O=CompanyName/OU=CompanySectionName/CN=CommonNameOrHostname"
openssl x509 -req -in cert1_req.pem -days 50000 -CA cert_ca.pem -CAkey key_ca.pem -CAcreateserial -out cert1.pem
openssl x509 -req -in cert2_req.pem -days 50000 -CA cert_ca.pem -CAkey key_ca.pem -CAcreateserial -out cert2.pem
openssl x509 -req -in cert3_req.pem -days 50000 -CA cert_ca.pem -CAkey key_ca.pem -CAcreateserial -out cert3.pem
```
