# in-situMQTT
In-situ MQTT Parser

The MQTT parsing and building software here uses in-situ parsing approach which do not allocate other buffer for string-based data to copy. Instead, it decodes string-based data by indicating the position of data at the raw-data received from network. For a figured explanation of in-situ parsing approach, refer to https://rapidjson.org/md_doc_dom.html. As different from the method used for RapidJSON (and probably for RapidXML), we do not modify the base data (raw-data) with null-character. In this way method can also be used for non-printible octet-strings. We consider that, this approach minimizes memory allocation and copying overheads, improves cache coherence so the performance. 

For building, after building completed, string data pointers are re-set to indicate data positions at the raw-data that has been built. So the external data passed during the building can be deallocated from the memory. After that the mqtt_message instance reflects a compact structure which is independent of external data as similar to the message instances which are used for parsing (refer to test function named test_for_insitu_on_building() in testhead.c file.
