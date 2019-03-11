CREATE DATABASE CTAonline;

USE CTAonline;

GRANT ALL PRIVILEGES ON CTAonline.* TO 'CTAreadwrite'@'%' IDENTIFIED BY 'write2db4pSCT';
GRANT ALL PRIVILEGES ON CTAonline.* TO 'CTAreadwrite'@'localhost' IDENTIFIED BY 'write2db4pSCT';

CREATE TABLE Opt_MPESStatus (date  datetime  NULL,
  serial_number smallint(6)  NULL ,
  status_code tinyint(3) unsigned);
