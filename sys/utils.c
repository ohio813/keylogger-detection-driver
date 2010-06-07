/*
 * Module: obutils.c
 * Author: Christopher Wood
 * Description: Outbound utililty and helper functions.
 * Environment: Kernel mode only
 */
 
 /* traverseQueue */
 void traverseQueue(KEY_QUEUE *queue)
{
    // Local variables
    LIST_ENTRY *pEntry = &queue->QueueListHead;
    LIST_ENTRY *pListHead;
    KEY_DATA *pKeyData;
    bool go = true;
    int counter = 0;

    // Save head in the list
    pListHead = pEntry;

    DbgPrint("Displaying queue's elements:\n");
    while (go) {
        pKeyData = CONTAINING_RECORD(pEntry, KEY_DATA, ListEntry);
        DbgPrint("%d.) %c", counter++, pKeyData->Data);
        pEntry = pEntry->Flink;
        if (pEntry == pListHead) // went through one loop in list
            go = false;
    }
}