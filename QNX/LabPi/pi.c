          char(lOO[16<<                               15])                        ;void
      l00(int(l0),int(ll)                         ){int(l)                      =l0%9,O
    =l%8      ?l==7?16:l%2                    ?4+l/2:-1:1-                     l/8,l2=O
  ==            1?1:O<1?O+                 3:O/14,l3=O%16<                   1?0:O/5?1:
                (4*O)%14,                l4=   (l3?l0/9*8:                  0)+O,l7=l3+
                (ll*l2)*                        (l2/2),l00                =l4>0?l7:-5*(
               l4*=2)                           ;char*l5=(               lOO +ll+ll*l2)
            ;while                              (l4>0?!l7?             l00   =(l00*10+*
        l5),*l5=(l00/l4)                        ,l00-=l4**            l5     ++:((*l5)=
          l00/l4,l00-=l4**                      l5++,l00*=          10       ):(l00+=(*
              --l5)+(l4+1)*                     lOO[--l7],        *l5        =(l00%10),
                l00/=10,(l00                    -=9*l4/(2)       )),         --ll);}int
                 myPi(int(l)                    ,char**O){     int           l0,ll=l0=(
                 l<1)<<30,l2                    =l==2?7+(*    O=1[O],myPi(64+myPi(63,O),O+1
                 )):(l==1)?l                    +++56:-64+    l;if(l!=2)return(l<2?!l?myPi(
                 10,O):(myPi                    (48+**O,O)    ,(l=(!(l&l0)?(myPi(46,O),ll|l
  ):l),          myPi(++l,(     ++*O,O          )))):l<62?                   my_putc(l)
:l>>6?(l        =**O)?l2*(    l-48)+myPi        (l2/10+64,                   (++*O,O)):
0:*(*O+1)?    10*(myPi(       63,((++(*O        ),O)))):1)                   ;while(ll=
 ll>=(l0%9==7?l0/9:1)         ?(l0++,0):      (l00(l0,l2),ll                 +1),l0<l2*
    9);return(myPi              ((6-l2    )&~((1<<30)),(*O=lOO+3             *l2,O)));}
