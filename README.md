unbuffredir
===========

Small utility for unbuffered redir for linux.

Example:

$ nice -n 20 ionice -c 3 mysqldump --database mydatabase --password="Very secret passwd" -u root \

 --extended-insert --single-transaction --comments -q | nice -n 20 ionice -c 3 \
 
 ./unbuffredir /backup/dump-at-morning.sql
