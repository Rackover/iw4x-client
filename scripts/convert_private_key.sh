 #!/bin/bash         

echo "Exporting DER rsa-private.key to PEM..."
openssl rsa -in rsa-private.key -inform DER -outform PEM -out exported-rsa-private.key
