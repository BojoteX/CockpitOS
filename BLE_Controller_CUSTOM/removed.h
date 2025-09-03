


/*
  // keep this line as-is
  featureCtrl = hid->featureReport(0);  // Feature, Report ID 0

  // remove the encryption gate so Windows can GET/SET FEATURE immediately
  featureCtrl->setAccessPermissions(ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE);
  // prime 64B value so first GET has data
  static uint8_t zeros64[64] = {0};
  featureCtrl->setValue(zeros64, 64);
  featureCtrl->setCallbacks(new FeatureCallbacks());

  // REQUIRED for Windows HID over GATT → route GET/SET_FEATURE to this char:
  static uint8_t rpt_ref_feat[] = { 0x00, 0x03 };  // ReportID=0, ReportType=Feature(3)
  BLEDescriptor* rr_feat = new BLEDescriptor(BLEUUID((uint16_t)0x2908));
  rr_feat->setValue(rpt_ref_feat, sizeof(rpt_ref_feat));
  featureCtrl->addDescriptor(rr_feat);  
*/