#include "SubmissionBatch.h"
namespace Graphics
{
void SubmissionBatch::clear()
{
    this->submissionEntries.clear(); 
    this->submissionSignalSemaphores.clear(); 
    this->submissionWaitSemaphores.clear();
    this->submissionWaitStages.clear();
}
}