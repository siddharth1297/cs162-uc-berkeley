#pragma once

/* Represent, which one is redirected 
 * -1 : input, 1 : output, 0: no redirection
 */ 
int redirect_opt;

/* Check and does reirect operations */
int redirect(struct tokens*, size_t);