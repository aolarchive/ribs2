#ifndef _TEMPLATE__H_
#define _TEMPLATE__H_

#define _TEMPLATE_HELPER(S,T) S ## _ ## T
#define TEMPLATE(S,T) _TEMPLATE_HELPER(S,T)

#define _TEMPLATE_FUNC_HELPER(S,T,F) S ## _ ## T ## _ ## F
#define TEMPLATE_FUNC(S,T,F) _TEMPLATE_FUNC_HELPER(S,T,F)

#define _STRINGIFY(T) #T
#define STRINGIFY(T) _STRINGIFY(T)


#endif // _TEMPLATE__H_
