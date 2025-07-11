#include <Feeder_Features.h>

FeedingWindow getActiveFeedingWindow(const Pet &pet)
{
    rtc.refresh();
    uint16_t now = rtc.hour() * 60 + rtc.minute();

    uint16_t start1 = pet.startHour1 * 60 + pet.startMinute1;
    uint16_t end1 = pet.endHour1 * 60 + pet.endMinute1;
    uint16_t start2 = pet.startHour2 * 60 + pet.startMinute2;
    uint16_t end2 = pet.endHour2 * 60 + pet.endMinute2;

    if (now >= start1 && now < end1)
        return WINDOW_1;
    if (now >= start2 && now < end2)
        return WINDOW_2;

    return NONE;
}
