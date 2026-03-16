# เลย์เอาต์แถวแนวนอน (Flex Row Layout)

ตัวอย่างนี้สาธิตการจัดเลย์เอาต์แบบ flex ของ LVGL ในโหมดแถว ปุ่มสี่ตัวถูกวางใน container ด้วย `LV_FLEX_FLOW_ROW` ทำให้เรียงตัวแนวนอนอัตโนมัติพร้อมระยะห่างที่เหมาะสม

ช่องว่างระหว่างปุ่มควบคุมด้วย `lv_obj_set_style_pad_column` ซึ่งกำหนด padding แนวนอนระหว่าง flex item ปุ่มถูกจัดกลางแนวตั้งภายในแถวโดยใช้พารามิเตอร์ cross-axis alignment ของ `lv_obj_set_flex_align`

แต่ละปุ่มมีสีแตกต่างจาก Palette ของ LVGL ทำให้เลย์เอาต์ชัดเจนทางภาพ flex container จัดการตำแหน่งโดยอัตโนมัติ ปุ่มจึงไม่ต้องกำหนดพิกัด x/y อย่างชัดเจน

flex layout เป็นแนวทางที่แนะนำสำหรับการออกแบบ UI แบบ responsive ใน LVGL รองรับทั้ง TESAIoT Dev Kit, PSoC Edge AI Kit และ PSoC Edge Eva Kit
