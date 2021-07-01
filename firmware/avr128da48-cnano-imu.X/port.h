/* 
 * File:   port.h
 * Author: M53740
 *
 * Created on 22. februar 2021, 09:55
 */

#ifndef PORT_H
#define	PORT_H

#ifdef	__cplusplus
extern "C" {
#endif
    
#include <avr/io.h>

void PORT_init();
    
#ifdef	__cplusplus
}
#endif

#endif	/* PORT_H */